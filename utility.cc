// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "utility.h"

#include "hwregs.h"

unsigned long ResetEntryInt() {
  register int cmd __asm__("$9") = 0x18;
  __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
  return ((int (*)(void))0xB0)();
}

extern const unsigned long __rom_mode;
int IsROM() { return __rom_mode; }

int HasCaetla() {
  // The "&" symbol from the "Unirom & Caetla are not licensed..."
  // vs "Unirom is not licensed"
  // If it's there, caetla's there.
  return (*(volatile char *)0x1F000038 == '&');
}

void Reboot() {
  // reset the machine e.g. immediately
  // after flashing a ROM.
  *(ulong *)ISTAT = 0;
  *(ulong *)IMASK = 0;
  goto *(ulong *)0xBFC00000;
}

void UnloadMe() {

  *(ulong *)0xBF801070 = 0;
  *(ulong *)0xBF801074 = 0;
}

// This magic number will cause the
// rom to just let the normal boot
// sequence happen next time
// Note: more of a unirom feature, but useful for xstation testing
int RebootToShell() {
  *(ulong *)0x80100060 = 0xFFEE2244;
  goto *(ulong *)0xBFC00000;
  return 0;
}

void AddDevice(void *deviceInfo) {
  register int cmd __asm__("$9") = 0x47;
  __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
  return ((void (*)(void *))0xB0)(deviceInfo);
}

// lowercase
void RemoveDevice(const std::string_view deviceName) {
  register int cmd __asm__("$9") = 0x48;
  __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
  return ((void (*)(const char *))0xB0)(deviceName.data());
}

void PrintDevices() {
  register int cmd __asm__("$9") = 0x49;
  __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
  return ((void (*)(void))0xB0)();
}

void CloseFile(ulong fileHandle) {
  register int cmd __asm__("$9") = 0x36;
  __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
  ((void (*)(ulong))0xB0)(fileHandle);
}

ulong OpenFile(const std::string_view fileName, ulong accessMode) {
  register int cmd __asm__("$9") = 0x32;
  __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
  return ((ulong(*)(const char *, ulong))0xB0)(fileName.data(), accessMode);
}

void FlushCache() {
  register int cmd asm("t1") = 0x44;
  __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
  return ((void (*)())0xA0)();
}

#pragma GCC push options
#pragma GCC optimize("-O0")
void Delay(int inLen) {
  int i = 0;
  for (i = 0; i < inLen; i++) {
    __asm__ volatile("" : "=r"(i) : "r"(i));
  }
}
#pragma GCC pop options

// returns the last n chars of a path
const char *GetFilenameFromFullPath(const char *inPath) {

  if (strlen(inPath) == 0) {
    return (char *)&"Error getting path from filename...";
  }

  const char *startPoint = inPath;
  const char *readHead = inPath;

  // will error out if a path+filename ends in / or
  while (*readHead != 0) {
    if (*readHead == '/' || *readHead == '\\') {
      startPoint = ++readHead;
    }
    readHead++;
  }

  return startPoint;
}

// get the last n characters of a file name/path
const char *GetTruncatedFilenameFromEnd(const char *inPath, int maxLength) {

  int len = strlen(inPath);

  if (len == 0) {
    return (char *)&"Error getting path from filename...";
  }

  const char *startPoint = inPath;

  if (len > maxLength - 3) {
    startPoint = (startPoint + len - maxLength - 3);
  }

  return startPoint;
}