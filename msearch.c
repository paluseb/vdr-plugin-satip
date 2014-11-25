/*
 * msearch.c: SAT>IP plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "common.h"
#include "discover.h"
#include "poller.h"
#include "msearch.h"

const char *cSatipMsearch::bcastAddressS = "239.255.255.250";
const char *cSatipMsearch::bcastMessageS = "M-SEARCH * HTTP/1.1\r\n"                  \
                                           "HOST: 239.255.255.250:1900\r\n"           \
                                           "MAN: \"ssdp:discover\"\r\n"               \
                                           "ST: urn:ses-com:device:SatIPServer:1\r\n" \
                                           "MX: 2\r\n\r\n";

cSatipMsearch::cSatipMsearch(void)
: bufferLenM(eProbeBufferSize),
  bufferM(MALLOC(unsigned char, bufferLenM)),
  registeredM(false)
{
  if (bufferM)
     memset(bufferM, 0, bufferLenM);
  else
     error("Cannot create Msearch buffer!");
  if (!Open(eDiscoveryPort))
     error("Cannot open Msearch port!");
}

cSatipMsearch::~cSatipMsearch()
{
}

void cSatipMsearch::Probe(void)
{
  debug("cSatipMsearch::%s()", __FUNCTION__);
  if (!registeredM) {
     cSatipPoller::GetInstance()->Register(*this);
     registeredM = true;
     }
  Write(bcastAddressS, reinterpret_cast<const unsigned char *>(bcastMessageS), strlen(bcastMessageS));
}

int cSatipMsearch::GetFd(void)
{
  return Fd();
}

void cSatipMsearch::Process(void)
{
  //debug("cSatipMsearch::%s()", __FUNCTION__);
  if (bufferM) {
     int length = Read(bufferM, bufferLenM);
     if (length > 0) {
        bufferM[min(length, int(bufferLenM - 1))] = 0;
        //debug("cSatipMsearch::%s(): len=%d buf=%s", __FUNCTION__, length, bufferM);
        bool status = false, valid = false;
        char *s, *p = reinterpret_cast<char *>(bufferM), *location = NULL;
        char *r = strtok_r(p, "\r\n", &s);
        while (r) {
              //debug("cSatipMsearch::%s(): %s", __FUNCTION__, r);
              // Check the status code
              // HTTP/1.1 200 OK
              if (!status && startswith(r, "HTTP/1.1 200 OK"))
                 status = true;
              if (status) {
                 // Check the location data
                 // LOCATION: http://192.168.0.115:8888/octonet.xml
                 if (startswith(r, "LOCATION:")) {
                    location = compactspace(r + 9);
                    debug("cSatipMsearch::%s(): location='%s'", __FUNCTION__, location);
                    }
                 // Check the source type
                 // ST: urn:ses-com:device:SatIPServer:1
                 else if (startswith(r, "ST:")) {
                    char *st = compactspace(r + 3);
                    if (strstr(st, "urn:ses-com:device:SatIPServer:1"))
                       valid = true;
                    debug("cSatipMsearch::%s(): st='%s'", __FUNCTION__, st);
                    }
                 // Check whether all the required data is found
                 if (valid && !isempty(location)) {
                    cSatipDiscover::GetInstance()->Probe(location);
                    break;
                    }
                 }
              r = strtok_r(NULL, "\r\n", &s);
              }
         }
     }
}
