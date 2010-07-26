/*
* Copyright (c) 2008, AMT – The Association For Manufacturing Technology (“AMT”)
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the AMT nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* DISCLAIMER OF WARRANTY. ALL MTCONNECT MATERIALS AND SPECIFICATIONS PROVIDED
* BY AMT, MTCONNECT OR ANY PARTICIPANT TO YOU OR ANY PARTY ARE PROVIDED "AS IS"
* AND WITHOUT ANY WARRANTY OF ANY KIND. AMT, MTCONNECT, AND EACH OF THEIR
* RESPECTIVE MEMBERS, OFFICERS, DIRECTORS, AFFILIATES, SPONSORS, AND AGENTS
* (COLLECTIVELY, THE "AMT PARTIES") AND PARTICIPANTS MAKE NO REPRESENTATION OR
* WARRANTY OF ANY KIND WHATSOEVER RELATING TO THESE MATERIALS, INCLUDING, WITHOUT
* LIMITATION, ANY EXPRESS OR IMPLIED WARRANTY OF NONINFRINGEMENT,
* MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 

* LIMITATION OF LIABILITY. IN NO EVENT SHALL AMT, MTCONNECT, ANY OTHER AMT
* PARTY, OR ANY PARTICIPANT BE LIABLE FOR THE COST OF PROCURING SUBSTITUTE GOODS
* OR SERVICES, LOST PROFITS, LOSS OF USE, LOSS OF DATA OR ANY INCIDENTAL,
* CONSEQUENTIAL, INDIRECT, SPECIAL OR PUNITIVE DAMAGES OR OTHER DIRECT DAMAGES,
* WHETHER UNDER CONTRACT, TORT, WARRANTY OR OTHERWISE, ARISING IN ANY WAY OUT OF
* THIS AGREEMENT, USE OR INABILITY TO USE MTCONNECT MATERIALS, WHETHER OR NOT
* SUCH PARTY HAD ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.
*/

#include "adapter.hpp"
#include "device.hpp"

using namespace std;

/* Adapter public methods */
Adapter::Adapter(
    const string& device,
    const string& server,
    const unsigned int port
  )
: Connector(server, port), mDevice(device)
{
}

Adapter::~Adapter()
{
  // Will stop threaded object gracefully Adapter::thread()
  stop();
  wait();
}

/**
 * Expected data to parse in SDHR format:
 *   Time|Alarm|Code|NativeCode|Severity|State|Description
 *   Time|Item|Value
 *   Time|Item1|Value1|Item2|Value2...
 */

void Adapter::processData(const string& data)
{
  istringstream toParse(data);
  string key;
  
  getline(toParse, key, '|');
  string time = key;
  
  getline(toParse, key, '|');
  string type = key;
  
  string value;
  getline(toParse, value, '|');

  DataItem *dataItem = mAgent->getDataItemByName(mDevice, key);
  if (dataItem == NULL)
  {
    logEvent("Agent", "Could not find data item: " + key);
  }
  else
  {
    string rest;
    if (dataItem->isCondition() || dataItem->isAlarm() || dataItem->isMessage())
    {
      getline(toParse, rest);
      value = value + "|" + rest;
    }

    // Add key->value pairings
    dataItem->setDataSource(this);
    mAgent->addToBuffer(dataItem, value, time);
  }
  
  // Look for more key->value pairings in the rest of the data
  while (getline(toParse, key, '|') && getline(toParse, value, '|'))
  {
    dataItem = mAgent->getDataItemByName(mDevice, key);
    if (dataItem == NULL)
    {
      logEvent("Agent", "Could not find data item: " + key);
    }
    else
    {
      dataItem->setDataSource(this);
      mAgent->addToBuffer(dataItem, toUpperCase(value), time);
    }
  }
}

inline static void trim(std::string str)
{
  size_t index = str.find_first_not_of(" \t");
  if (index != string::npos)
    str.erase(0, index);
  index = str.find_last_not_of(" \t");
  if (index != string::npos)
    str.erase(index);
}

void Adapter::protocolCommand(const std::string& data)
{
  Device *device = mAgent->getDeviceByName(mDevice);
  if (device != NULL)
  {
    if (data.compare(0, 7, "* PROBE") == 0)
    {
      string probe = mAgent->handleProbe(mDevice);
      long status = mConnection->write(probe.c_str(), probe.length());
      if (status < 0)
      {
        logEvent("Adapter::protocolCommand", "Could not write probe  " + intToString(status));
      }
    }
    else 
    {
      // Handle initial push of settings for uuid, serial number and manufacturer. 
      // This will override the settings in the device from the xml
      size_t index = data.find(':', 2);
      if (index != string::npos)
      {
        // Slice from the second character to the :, without the colon
        string key = data.substr(2, index - 2);
        trim(key);        
        string value = data.substr(index + 1);
        trim(value);
        
        if (key == "uuid")
          device->setUuid(value);
        else if (key == "manufacturer")
          device->setManufacturer(value);
        else if (key == "station")
          device->setStation(value);
        else if (key == "serialNumber")
          device->setSerialNumber(value);
        else
          logEvent("Agent", "Unknown command '" + data + "' for device '" + mDevice);
      }
    }
  }
}

void Adapter::disconnected()
{
  mAgent->disconnected(this, mDevice);
}

/* Adapter private methods */
void Adapter::thread()
{
  // Start the connection to the socket
  while (true)
  {
    connect();
    // Try to reconnect every 10 seconds
    dlib::sleep(10 * 1000);
  }
}

