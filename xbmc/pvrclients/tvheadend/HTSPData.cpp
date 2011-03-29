/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "HTSPData.h"

extern "C" {
#include "libhts/htsmsg.h"
#include "libhts/htsmsg_binary.h"
}

#define CMD_LOCK cMutexLock CmdLock((cMutex*)&m_Mutex)

cHTSPData::cHTSPData()
{
}

cHTSPData::~cHTSPData()
{
  Close();
}

bool cHTSPData::Open(CStdString hostname, int port, CStdString user, CStdString pass, long timeout)
{
  if(!m_session.Connect(hostname, port))
    return false;

  if(m_session.GetProtocol() < 2)
  {
    XBMC->Log(LOG_ERROR, "%s - Incompatible protocol version %d", __FUNCTION__, m_session.GetProtocol());
    return false;
  }

  if(!user.IsEmpty())
    m_session.Auth(user, pass);

  SetDescription("HTSP Data Listener");
  Start();

  m_started.Wait(timeout);
  return Running();
}

void cHTSPData::Close()
{
  Cancel(3);
  m_session.Abort();
  m_session.Close();
}

bool cHTSPData::CheckConnection()
{
  if (!m_session.IsConnected())
    m_session.Connect(g_szHostname, g_iPortHTSP);

  return m_session.IsConnected();
}

htsmsg_t* cHTSPData::ReadResult(htsmsg_t* m)
{
  m_Mutex.Lock();
  unsigned    seq (m_session.AddSequence());

  SMessage &message(m_queue[seq]);
  message.event = new cCondWait();
  message.msg   = NULL;

  m_Mutex.Unlock();
  htsmsg_add_u32(m, "seq", seq);
  if(!m_session.SendMessage(m))
  {
    m_queue.erase(seq);
    return NULL;
  }

  if(!message.event->Wait(2000))
  {
    XBMC->Log(LOG_ERROR, "%s - Timeout waiting for response", __FUNCTION__);
    m_session.Close();
  }
  m_Mutex.Lock();

  m =    message.msg;
  delete message.event;

  m_queue.erase(seq);

  m_Mutex.Unlock();
  return m;
}

bool cHTSPData::GetDriveSpace(long long *total, long long *used)
{
  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "getDiskSpace");
  if ((msg = ReadResult(msg)) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "%s - failed to get getDiskSpace", __FUNCTION__);
    return false;
  }

  int64_t freespace;
  if (htsmsg_get_s64(msg, "freediskspace", &freespace) != 0)
    return false;

  int64_t totalspace;
  if (htsmsg_get_s64(msg, "totaldiskspace", &totalspace) != 0)
    return false;

  *total = totalspace / 1024;
  *used  = (totalspace - freespace) / 1024;
  return true;
}

bool cHTSPData::GetTime(time_t *localTime, int *gmtOffset)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "getSysTime");
  if ((msg = ReadResult(msg)) == NULL)
  {
    XBMC->Log(LOG_ERROR, "%s - failed to get sysTime", __FUNCTION__);
    return false;
  }

  unsigned int secs;
  if (htsmsg_get_u32(msg, "time", &secs) != 0)
    return false;

  int offset;
  if (htsmsg_get_s32(msg, "timezone", &offset) != 0)
    return false;

  XBMC->Log(LOG_DEBUG, "%s - tvheadend reported time=%u, timezone=%d, correction=%d"
      , __FUNCTION__, secs, offset);

  *localTime = secs + offset;
  *gmtOffset = -offset;
  return true;
}

int cHTSPData::GetNumChannels()
{
  return GetChannels().size();
}

PVR_ERROR cHTSPData::GetChannels(PVR_HANDLE handle, bool bRadio)
{
  if (!CheckConnection())
    return PVR_ERROR_SERVER_ERROR;

  SChannels channels = GetChannels();
  for(SChannels::iterator it = channels.begin(); it != channels.end(); ++it)
  {
    SChannel& channel = it->second;
    if(bRadio != channel.radio)
      continue;

    PVR_CHANNEL tag;
    memset(&tag, 0 , sizeof(PVR_CHANNEL));

    tag.iUniqueId         = channel.id;
    tag.bIsRadio          = channel.radio;
    tag.iChannelNumber    = channel.num;
    tag.strChannelName    = channel.name.c_str();
    tag.strInputFormat    = "";
    tag.strStreamURL      = "";
    tag.iEncryptionSystem = channel.caid;
    tag.strIconPath       = channel.icon.c_str();
    tag.bIsHidden         = false;
    tag.bIsRecording      = false;

    PVR->TransferChannelEntry(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cHTSPData::GetEpg(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (!CheckConnection())
    return PVR_ERROR_SERVER_ERROR;

  SChannels channels = GetChannels();

  if (channels.find(channel.iUniqueId) != channels.end())
  {
    time_t stop;

    SEvent event;
    event.id = channels[channel.iUniqueId].event;
    if (event.id == 0)
      return PVR_ERROR_NO_ERROR;

    do
    {
      bool success = GetEvent(event, event.id);
      if (success)
      {
        EPG_TAG broadcast;
        memset(&broadcast, 0, sizeof(EPG_TAG));

        broadcast.iUniqueBroadcastId = event.id;
        broadcast.strTitle           = event.title.c_str();
        broadcast.iChannelNumber     = event.chan_id >= 0 ? event.chan_id : channel.iUniqueId;
        broadcast.startTime          = event.start;
        broadcast.endTime            = event.stop;
        broadcast.strPlotOutline     = "";
        broadcast.strPlot            = event.descs.c_str();
        broadcast.strIconPath        = "";
        broadcast.iGenreType         = (event.content & 0x0F) << 4;
        broadcast.iGenreSubType      = event.content & 0xF0;
        broadcast.firstAired         = 0;
        broadcast.iParentalRating    = 0;
        broadcast.iStarRating        = 0;
        broadcast.bNotify            = false;
        broadcast.iSeriesNumber         = 0;
        broadcast.iEpisodeNumber        = 0;
        broadcast.iEpisodePartNumber       = 0;
        broadcast.strEpisodeName     = "";

        PVR->TransferEpgEntry(handle, &broadcast);

        event.id = event.next;
        stop = event.stop;
      }
      else
        break;

    } while(iEnd > stop);

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_NO_ERROR;
}

SRecordings cHTSPData::GetDVREntries(bool recorded, bool scheduled)
{
  CMD_LOCK;
  SRecordings recordings;

  for(SRecordings::const_iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
  {
    SRecording recording = it->second;

    if ((recorded && (recording.state == ST_COMPLETED || recording.state == ST_ABORTED)) ||
        (scheduled && (recording.state == ST_SCHEDULED || recording.state == ST_RECORDING)))
      recordings[recording.id] = recording;
  }

  return recordings;
}

int cHTSPData::GetNumRecordings()
{
  SRecordings recordings = GetDVREntries(true, false);
  return recordings.size();
}

PVR_ERROR cHTSPData::GetRecordings(PVR_HANDLE handle)
{
  SRecordings recordings = GetDVREntries(true, false);

  for(SRecordings::const_iterator it = recordings.begin(); it != recordings.end(); ++it)
  {
    SRecording recording = it->second;

    CStdString strStreamURL = "http://";
    CStdString strChannelName = "";

    /* lock */
    {
      CMD_LOCK;
      SChannels::const_iterator itr = m_channels.find(recording.channel);
      if (itr != m_channels.end())
        strChannelName = itr->second.name.c_str();

      if (g_szUsername != "")
      {
        strStreamURL += g_szUsername;
        if (g_szPassword != "")
        {
          strStreamURL += ":";
          strStreamURL += g_szPassword;
        }
        strStreamURL += "@";
      }
      strStreamURL.Format("%s%s:%i/dvrfile/%i", strStreamURL.c_str(), g_szHostname.c_str(), g_iPortHTTP, recording.id);
    }

    PVR_RECORDING tag;
    memset(&tag, 0, sizeof(PVR_RECORDING));

    tag.iClientIndex   = recording.id;
    tag.strTitle       = recording.title.c_str();
    tag.strStreamURL   = strStreamURL.c_str();
    tag.strDirectory   = "/";
    tag.strPlotOutline = "";
    tag.strPlot        = recording.description.c_str();
    tag.strChannelName = strChannelName.c_str();
    tag.recordingTime  = recording.start;
    tag.iDuration      = recording.stop - recording.start;
    tag.iPriority      = 0;
    tag.iLifetime      = 0;

    PVR->TransferRecordingEntry(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cHTSPData::DeleteRecording(const PVR_RECORDING &recording)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "deleteDvrEntry");
  htsmsg_add_u32(msg, "id", recording.iClientIndex);
  if ((msg = ReadResult(msg)) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to get deleteDvrEntry", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  unsigned int success;
  if (htsmsg_get_u32(msg, "success", &success) != 0)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to parse param", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  return success > 0 ? PVR_ERROR_NO_ERROR : PVR_ERROR_NOT_DELETED;
}

int cHTSPData::GetNumTimers()
{
  SRecordings recordings = GetDVREntries(false, true);
  return recordings.size();
}

PVR_ERROR cHTSPData::GetTimers(PVR_HANDLE handle)
{
  SRecordings recordings = GetDVREntries(false, true);

  for(SRecordings::const_iterator it = recordings.begin(); it != recordings.end(); ++it)
  {
    SRecording recording = it->second;

    PVR_TIMER tag;
    memset(&tag, 0, sizeof(PVR_TIMER));

    tag.iClientIndex      = recording.id;
    tag.iClientChannelUid = recording.channel;
    tag.startTime         = recording.start;
    tag.endTime           = recording.stop;
    tag.strTitle          = recording.title.c_str();
    tag.strDirectory            = "/";
    tag.strSummary        = "";
    tag.bIsActive         = recording.state == ST_SCHEDULED || recording.state == ST_RECORDING;
    tag.bIsRecording      = recording.state == ST_RECORDING;
    tag.iPriority         = 0;
    tag.iLifetime         = 0;
    tag.bIsRepeating      = false;
    tag.firstDay          = 0;
    tag.iWeekdays         = 0;
    tag.iEpgUid           = 0;

    PVR->TransferTimerEntry(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cHTSPData::DeleteTimer(const PVR_TIMER &timer, bool bForce)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "deleteDvrEntry");
  htsmsg_add_u32(msg, "id", timer.iClientIndex);
  if ((msg = ReadResult(msg)) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to get deleteDvrEntry", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  unsigned int success;
  if (htsmsg_get_u32(msg, "success", &success) != 0)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to parse param", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  return success > 0 ? PVR_ERROR_NO_ERROR : PVR_ERROR_NOT_DELETED;
}

PVR_ERROR cHTSPData::AddTimer(const PVR_TIMER &timer)
{
  XBMC->Log(LOG_DEBUG, "%s - channelUid=%d title=%s epgid=%d", __FUNCTION__, timer.iClientChannelUid, timer.strTitle, timer.iEpgUid);

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method",      "addDvrEntry");
  htsmsg_add_u32(msg, "eventId",     timer.iEpgUid);
  htsmsg_add_str(msg, "title",       timer.strTitle);
  htsmsg_add_u32(msg, "start",       timer.startTime);
  htsmsg_add_u32(msg, "stop",        timer.endTime);
  htsmsg_add_u32(msg, "channelId",   timer.iClientChannelUid);
  htsmsg_add_u32(msg, "priority",    timer.iPriority);
  htsmsg_add_str(msg, "description", timer.strSummary);
  htsmsg_add_str(msg, "creator",     "XBMC");

  if ((msg = ReadResult(msg)) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to get addDvrEntry", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  const char *strError = NULL;
  if ((strError = htsmsg_get_str(msg, "error")))
  {
    XBMC->Log(LOG_DEBUG, "%s - Error adding timer: '%s'", __FUNCTION__, strError);
    return PVR_ERROR_SERVER_ERROR;
  }

  unsigned int success;
  if (htsmsg_get_u32(msg, "success", &success) != 0)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to parse param", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  return success > 0 ? PVR_ERROR_NO_ERROR : PVR_ERROR_NOT_DELETED;
}

PVR_ERROR cHTSPData::UpdateTimer(const PVR_TIMER &timer)
{
  XBMC->Log(LOG_DEBUG, "%s - channelUid=%d title=%s epgid=%d", __FUNCTION__, timer.iClientChannelUid, timer.strTitle, timer.iEpgUid);

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "updateDvrEntry");
  htsmsg_add_u32(msg, "id",     timer.iClientIndex);
  htsmsg_add_str(msg, "title",  timer.strTitle);
  htsmsg_add_u32(msg, "start",  timer.startTime);
  htsmsg_add_u32(msg, "stop",   timer.endTime);
  
  if ((msg = ReadResult(msg)) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to get updateDvrEntry", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  unsigned int success;
  if (htsmsg_get_u32(msg, "success", &success) != 0)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to parse param", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  return success > 0 ? PVR_ERROR_NO_ERROR : PVR_ERROR_NOT_SAVED;
}

PVR_ERROR cHTSPData::RenameRecording(const PVR_RECORDING &recording, const char *strNewName)
{
  XBMC->Log(LOG_DEBUG, "%s - id=%d", __FUNCTION__, recording.iClientIndex);

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "updateDvrEntry");
  htsmsg_add_u32(msg, "id",     recording.iClientIndex);
  htsmsg_add_str(msg, "title",  recording.strTitle);
  
  if ((msg = ReadResult(msg)) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to get updateDvrEntry", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  unsigned int success;
  if (htsmsg_get_u32(msg, "success", &success) != 0)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to parse param", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  return success > 0 ? PVR_ERROR_NO_ERROR : PVR_ERROR_NOT_SAVED;
}


void cHTSPData::Action()
{
  XBMC->Log(LOG_DEBUG, "%s - Starting", __FUNCTION__);

  htsmsg_t* msg;
  if(!m_session.SendEnableAsync())
  {
    XBMC->Log(LOG_ERROR, "%s - couldn't send EnableAsync().", __FUNCTION__);
    m_started.Signal();
    return;
  }

  while (Running())
  {
    if((msg = m_session.ReadMessage()) == NULL)
      break;

    uint32_t seq;
    if(htsmsg_get_u32(msg, "seq", &seq) == 0)
    {
      CMD_LOCK;
      SMessages::iterator it = m_queue.find(seq);
      if(it != m_queue.end())
      {
        it->second.msg = msg;
        it->second.event->Signal();
        continue;
      }
    }

    const char* method;
    if((method = htsmsg_get_str(msg, "method")) == NULL)
    {
      htsmsg_destroy(msg);
      continue;
    }

    CMD_LOCK;
    if     (strstr(method, "channelAdd"))
      cHTSPSession::ParseChannelUpdate(msg, m_channels);
    else if(strstr(method, "channelUpdate"))
      cHTSPSession::ParseChannelUpdate(msg, m_channels);
    else if(strstr(method, "channelDelete"))
      cHTSPSession::ParseChannelRemove(msg, m_channels);
    else if(strstr(method, "tagAdd"))
      cHTSPSession::ParseTagUpdate(msg, m_tags);
    else if(strstr(method, "tagUpdate"))
      cHTSPSession::ParseTagUpdate(msg, m_tags);
    else if(strstr(method, "tagDelete"))
      cHTSPSession::ParseTagRemove(msg, m_tags);
    else if(strstr(method, "initialSyncCompleted"))
      m_started.Signal();
    else if(strstr(method, "dvrEntryAdd"))
      cHTSPSession::ParseDVREntryUpdate(msg, m_recordings);
    else if(strstr(method, "dvrEntryUpdate"))
      cHTSPSession::ParseDVREntryUpdate(msg, m_recordings);
    else if(strstr(method, "dvrEntryDelete"))
      cHTSPSession::ParseDVREntryDelete(msg, m_recordings);
    else
      XBMC->Log(LOG_DEBUG, "%s - Unmapped action recieved '%s'", __FUNCTION__, method);

    htsmsg_destroy(msg);
  }

  m_started.Signal();
  XBMC->Log(LOG_DEBUG, "%s - Exiting", __FUNCTION__);
}

SChannels cHTSPData::GetChannels()
{
  return GetChannels(0);
}

SChannels cHTSPData::GetChannels(int tag)
{
  CMD_LOCK;
  if(tag == 0)
    return m_channels;

  STags::iterator it = m_tags.find(tag);
  if(it == m_tags.end())
  {
    SChannels channels;
    return channels;
  }
  return GetChannels(it->second);
}

SChannels cHTSPData::GetChannels(STag& tag)
{
  CMD_LOCK;
  SChannels channels;

  std::vector<int>::iterator it;
  for(it = tag.channels.begin(); it != tag.channels.end(); it++)
  {
    SChannels::iterator it2 = m_channels.find(*it);
    if(it2 == m_channels.end())
    {
      XBMC->Log(LOG_ERROR, "%s - tag points to unknown channel %d", __FUNCTION__, *it);
      continue;
    }
    channels[*it] = it2->second;
  }
  return channels;
}

STags cHTSPData::GetTags()
{
  CMD_LOCK;
  return m_tags;
}

bool cHTSPData::GetEvent(SEvent& event, uint32_t id)
{
  if(id == 0)
  {
    event.Clear();
    return false;
  }

  SEvents::iterator it = m_events.find(id);
  if(it != m_events.end())
  {
    event = it->second;
    return true;
  }

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "getEvent");
  htsmsg_add_u32(msg, "eventId", id);
  if((msg = ReadResult(msg)) == NULL)
  {
    XBMC->Log(LOG_DEBUG, "%s - failed to get event %u", __FUNCTION__, id);
    return false;
  }
  if(!cHTSPSession::ParseEvent(msg, id, event))
    return false;

  m_events[id] = event;
  return true;
}