#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://xbmc.org
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

<<<<<<< HEAD
=======
#include "system.h"

>>>>>>> 1495cbeb771bb5dde20a83a50d23c89a50e6f5c1
#if !defined(HAVE_LIBCEC)
#include "Peripheral.h"

// an empty implementation, so CPeripherals can be compiled without a bunch of #ifdef's when libCEC is not available
namespace PERIPHERALS
{
  class CPeripheralCecAdapter : public CPeripheral
  {
  public:
    bool HasConnectedAudioSystem(void) { return false; }
    void ScheduleVolumeUp(void) {}
    void ScheduleVolumeDown(void) {}
    bool IsMuted(void) { return false; }
    void ScheduleMute(void) {}

<<<<<<< HEAD
    WORD GetButton(void) { return 0; }
=======
    int GetButton(void) { return 0; }
>>>>>>> 1495cbeb771bb5dde20a83a50d23c89a50e6f5c1
    unsigned int GetHoldTime(void) { return 0; }
    void ResetButton(void) {}
  };
}

#else

#include "PeripheralHID.h"
#include "interfaces/AnnouncementManager.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include <queue>

// undefine macro isset, it collides with function in cectypes.h
#ifdef isset
#undef isset
#endif
#include <libcec/cectypes.h>

class DllLibCEC;

namespace CEC
{
  class ICECAdapter;
};

namespace PERIPHERALS
{
  class CPeripheralCecAdapterUpdateThread;

  typedef struct
  {
    int         iButton;
    unsigned int iDuration;
  } CecButtonPress;

  typedef enum
  {
    VOLUME_CHANGE_NONE,
    VOLUME_CHANGE_UP,
    VOLUME_CHANGE_DOWN,
    VOLUME_CHANGE_MUTE
  } CecVolumeChange;

  class CPeripheralCecAdapter : public CPeripheralHID, public ANNOUNCEMENT::IAnnouncer, private CThread
  {
    friend class CPeripheralCecAdapterUpdateThread;

  public:
    CPeripheralCecAdapter(const PeripheralType type, const PeripheralBusType busType, const CStdString &strLocation, const CStdString &strDeviceName, int iVendorId, int iProductId);
    virtual ~CPeripheralCecAdapter(void);

<<<<<<< HEAD
    void Announce(ANNOUNCEMENT::EAnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);
    bool HasConnectedAudioSystem(void);
    void SetAudioSystemConnected(bool bSetTo);
    void ScheduleVolumeUp(void);
    void VolumeUp(void);
    void ScheduleVolumeDown(void);
    void VolumeDown(void);
    void ScheduleMute(void);
    void Mute(void);
    bool IsMuted(void);

    void OnSettingChanged(const CStdString &strChangedSetting);
    void OnDeviceRemoved(void);
=======
    virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);
    virtual bool HasConnectedAudioSystem(void);
    virtual void SetAudioSystemConnected(bool bSetTo);
    virtual void ScheduleVolumeUp(void);
    virtual void VolumeUp(void);
    virtual void ScheduleVolumeDown(void);
    virtual void VolumeDown(void);
    virtual void ScheduleMute(void);
    virtual void Mute(void);
    virtual bool IsMuted(void);
>>>>>>> 1495cbeb771bb5dde20a83a50d23c89a50e6f5c1

    WORD GetButton(void);
    unsigned int GetHoldTime(void);
    void ResetButton(void);
    CStdString GetComPort(void);

<<<<<<< HEAD
    void PushCecKeypress(const CEC::cec_keypress &key);

  protected:
    bool OpenConnection(void);
    void SetConfigurationFromSettings(void);
    void SetConfigurationFromLibCEC(const CEC::libcec_configuration &config);
    void SetVersionInfo(const CEC::libcec_configuration &configuration);
    static void ReadLogicalAddresses(const CStdString &strString, CEC::cec_logical_addresses &addresses);
    static int CecKeyPress(void *cbParam, const CEC::cec_keypress &key);
    void PushCecKeypress(const CecButtonPress &key);
    static int CecLogMessage(void *cbParam, const CEC::cec_log_message &message);
    static int CecCommand(void *cbParam, const CEC::cec_command &command);
    static int CecConfiguration(void *cbParam, const CEC::libcec_configuration &config);
    static int CecAlert(void *cbParam, const CEC::libcec_alert alert, const CEC::libcec_parameter &data);
    bool IsRunning(void) const;
    void ReopenConnection(void);

    void GetNextKey(void);
    bool InitialiseFeature(const PeripheralFeature feature);
    void Process(void);
    void ProcessVolumeChange(void);
    void SetMenuLanguage(const char *strLanguage);
=======
    virtual int GetButton(void);
    virtual unsigned int GetHoldTime(void);
    virtual void ResetButton(void);
    virtual CStdString GetComPort(void);

  protected:
    virtual bool OpenConnection(void);
    virtual void SetConfigurationFromSettings(void);
    virtual void SetConfigurationFromLibCEC(const CEC::libcec_configuration &config);
    static void ReadLogicalAddresses(const CStdString &strString, CEC::cec_logical_addresses &addresses);
    static int CecKeyPress(void *cbParam, const CEC::cec_keypress &key);
    static int CecLogMessage(void *cbParam, const CEC::cec_log_message &message);
    static int CecCommand(void *cbParam, const CEC::cec_command &command);
    static int CecConfiguration(void *cbParam, const CEC::libcec_configuration &config);

    virtual bool GetNextKey(void);
    virtual bool GetNextCecKey(CEC::cec_keypress &key);
    virtual bool InitialiseFeature(const PeripheralFeature feature);
    virtual void Process(void);
    virtual void ProcessVolumeChange(void);
    virtual void SetMenuLanguage(const char *strLanguage);
>>>>>>> 1495cbeb771bb5dde20a83a50d23c89a50e6f5c1
    static bool FindConfigLocation(CStdString &strString);
    static bool TranslateComPort(CStdString &strPort);

    DllLibCEC*                        m_dll;
    CEC::ICECAdapter*                 m_cecAdapter;
    bool                              m_bStarted;
    bool                              m_bHasButton;
    bool                              m_bIsReady;
    bool                              m_bHasConnectedAudioSystem;
    CStdString                        m_strMenuLanguage;
    CDateTime                         m_screensaverLastActivated;
<<<<<<< HEAD
    std::vector<CecButtonPress>       m_buttonQueue;
    CecButtonPress                    m_currentButton;
=======
    CecButtonPress                    m_button;
    std::queue<CEC::cec_keypress>     m_buttonQueue;
>>>>>>> 1495cbeb771bb5dde20a83a50d23c89a50e6f5c1
    std::queue<CecVolumeChange>       m_volumeChangeQueue;
    unsigned int                      m_lastKeypress;
    CecVolumeChange                   m_lastChange;
    int                               m_iExitCode;
    bool                              m_bIsMuted;
<<<<<<< HEAD
    bool                              m_bGoingToStandby;
    bool                              m_bIsRunning;
    bool                              m_bDeviceRemoved;
=======
>>>>>>> 1495cbeb771bb5dde20a83a50d23c89a50e6f5c1
    CPeripheralCecAdapterUpdateThread*m_queryThread;
    CEC::ICECCallbacks                m_callbacks;
    CCriticalSection                  m_critSection;
    CEC::libcec_configuration         m_configuration;
  };

  class CPeripheralCecAdapterUpdateThread : public CThread
  {
  public:
    CPeripheralCecAdapterUpdateThread(CPeripheralCecAdapter *adapter, CEC::libcec_configuration *configuration);
    virtual ~CPeripheralCecAdapterUpdateThread(void);

<<<<<<< HEAD
    void Signal(void);
    bool UpdateConfiguration(CEC::libcec_configuration *configuration);

  protected:
    void UpdateMenuLanguage(void);
    CStdString UpdateAudioSystemStatus(void);
    bool WaitReady(void);
    bool SetInitialConfiguration(void);
    void Process(void);
=======
    virtual void Signal(void);
    virtual bool UpdateConfiguration(CEC::libcec_configuration *configuration);

  protected:
    virtual bool WaitReady(void);
    virtual bool SetInitialConfiguration(void);
    virtual void Process(void);
>>>>>>> 1495cbeb771bb5dde20a83a50d23c89a50e6f5c1

    CPeripheralCecAdapter *    m_adapter;
    CEvent                     m_event;
    CCriticalSection           m_critSection;
    CEC::libcec_configuration  m_configuration;
    CEC::libcec_configuration  m_nextConfiguration;
    bool                       m_bNextConfigurationScheduled;
    bool                       m_bIsUpdating;
  };
}

#endif
