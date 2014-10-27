/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011-2013 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/CTegraCECAdapterCommunication
 *     http://www.pulse-eight.net/
 */

#include "env.h"

#if defined(HAVE_TEGRA_API)
#include "TegraCECAdapterCommunication.h"
#include "TegraCECDev.h"

#include "lib/CECTypeUtils.h"
#include "lib/LibCEC.h"
#include "lib/platform/sockets/cdevsocket.h"
#include "lib/platform/util/StdString.h"
#include "lib/platform/util/buffer.h"

using namespace std;
using namespace CEC;
using namespace PLATFORM;

#include "AdapterMessageQueue.h"

#define LIB_CEC m_callback->GetLib()


TegraCECAdapterCommunication::TegraCECAdapterCommunication(IAdapterCommunicationCallback *callback) :
    IAdapterCommunication(callback),
    m_bLogicalAddressChanged(false)
{ 
  CLockObject lock(m_mutex);
  LIB_CEC->AddLog(CEC_LOG_ERROR, "%s: Creating Adaptor", __func__);
  m_iNextMessage = 0;
  m_logicalAddresses.Clear();
}


TegraCECAdapterCommunication::~TegraCECAdapterCommunication(void)
{
  Close();
  CLockObject lock(m_mutex);
  delete m_dev;
  m_dev = 0;
}


bool TegraCECAdapterCommunication::IsOpen(void)
{
  return devOpen;
}

    
bool TegraCECAdapterCommunication::Open(uint32_t iTimeoutMs, bool UNUSED(bSkipChecks), bool bStartListening)
{

    
  fd = open(TEGRA_CEC_DEV_PATH, O_RDWR);

  if (fd < 0){
    LIB_CEC->AddLog(CEC_LOG_ERROR, "%s: Failed To Open Tegra CEC Device", __func__);
  } else {
    if (!bStartListening || CreateThread())
    devOpen = true;
    return true;
  }

  return true;
}


void TegraCECAdapterCommunication::Close(void)
{
  StopThread(0);
  close(fd);
}


std::string TegraCECAdapterCommunication::GetError(void) const
{
  std::string strError(m_strError);
  return strError;
}


cec_adapter_message_state TegraCECAdapterCommunication::Write(
  const cec_command &data, bool &UNUSED(bRetry), uint8_t UNUSED(iLineTimeout), bool UNUSED(bIsReply))
{

  int size = 0;
  unsigned char cmdData[TEGRA_CEC_FRAME_MAX_LENGTH];
  unsigned char addr = (data.initiator << 4) | (data.destination & 0x0f); 


  if (data.initiator == data.destination){
    return ADAPTER_MESSAGE_STATE_SENT_NOT_ACKED;
  }

  cmdData[size] = addr;
  size++;

  if (data.opcode_set){
     cmdData[size] = data.opcode;
     size++;
  }

  

  for (int i = 0; i < data.parameters.size; i++){
    cmdData[size] = data.parameters.data[i];
    size++;

    if (size > TEGRA_CEC_FRAME_MAX_LENGTH){
      return ADAPTER_MESSAGE_STATE_SENT_NOT_ACKED;
    }
  }

  int status = write(fd,cmdData,size);

  if (status < 0){

    if(errno == ECONNRESET || errno == EHOSTUNREACH){
      return ADAPTER_MESSAGE_STATE_SENT_NOT_ACKED;
    } else {
      return ADAPTER_MESSAGE_STATE_ERROR;
    }

  } else {
    return ADAPTER_MESSAGE_STATE_SENT_ACKED;
  }
    
}


uint16_t TegraCECAdapterCommunication::GetFirmwareVersion(void)
{
  return 0;
}


cec_vendor_id TegraCECAdapterCommunication::GetVendorId(void)
{
   return CEC_VENDOR_LG;  
}


cec_logical_addresses TegraCECAdapterCommunication::GetLogicalAddresses(void)
{
  CLockObject lock(m_mutex);

  if (m_bLogicalAddressChanged || m_logicalAddresses.IsEmpty() )
  {
    m_logicalAddresses.Clear();
      
    for (int la = CECDEVICE_TV; la < CECDEVICE_BROADCAST; la++)
    {
      m_logicalAddresses.Set(cec_logical_address(la));  
    }

    m_bLogicalAddressChanged = false;
  }

  return m_logicalAddresses;
}


bool TegraCECAdapterCommunication::SetLogicalAddresses(const cec_logical_addresses &addresses)
{  
  return true;
}


void TegraCECAdapterCommunication::HandleLogicalAddressLost(cec_logical_address UNUSED(oldAddress))
{
}


void *TegraCECAdapterCommunication::Process(void)
{
  unsigned char opcode;
  cec_logical_address initiator, destination;

  while (!IsStopped())
  {

    int8_t isNotEndOfData = 1;
    unsigned char buffer[2] = {0,0};
    cec_command cmd;

    int rc = read(fd,buffer,2);
    initiator = cec_logical_address(buffer[0] >> 4);
    destination = cec_logical_address(buffer[0] & 0x0f);

    if ((buffer[1] & 0x01) > 0){
      isNotEndOfData = 0;
    }

    if (isNotEndOfData > 0){
     rc = read(fd,buffer,2);
     opcode = buffer[0];
     cec_command::Format(cmd, initiator, destination, cec_opcode(opcode));
      if ((buffer[1] & 0x01) > 0){
        isNotEndOfData = 0;
      }
    } else {
     cec_command::Format(cmd, initiator, destination, CEC_OPCODE_NONE);
    }

    while (isNotEndOfData > 0){

      if (read(fd,buffer,2) < 0){
        LIB_CEC->AddLog(CEC_LOG_TRAFFIC, "%s: Failed To Read From Tegra CEC Device", __func__);
      }

      cmd.parameters.PushBack(buffer[0]);

      if ((buffer[1] & 0x01) > 0){
        isNotEndOfData = 0;
      }

    }

    //LIB_CEC->AddLog(CEC_LOG_TRAFFIC, "%s: Reading Data Len : %i", __func__, cmd.parameters.size);
    if (!IsStopped())
      m_callback->OnCommandReceived(cmd);
  }

  return 0;
}

#endif
