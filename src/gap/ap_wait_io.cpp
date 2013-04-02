/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2013-2013 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#include "ap_defs.h"
#include "ap_wait_io.h"

#ifndef WIN32
#include <errno.h>
#include <poll.h>
#endif

namespace ap {


WaitIO::WaitIO(FXIODevice * device,FXInputHandle w,FXTime tm) : FXIO(device->mode()),io(device),watch(w),timeout(tm) {
	}

WaitIO::~WaitIO() {
	close();
	}

FXbool WaitIO::isOpen() const {
	return io->isOpen();
	}

FXbool WaitIO::isSerial() const {
	return io->isSerial();
	}

FXlong WaitIO::position() const {
	return io->position();
	}

FXlong WaitIO::position(FXlong offset,FXuint from) {
	return io->position(offset,from);
	}

FXival WaitIO::writeBlock(const void* data,FXival count){
	FXival n;
	do {
		n = io->writeBlock(data,count);
		}
	while(n<0 && ((errno==EWOULDBLOCK || errno==EAGAIN) && wait(WaitIO::Readable)));
	return n;
	}

FXival WaitIO::readBlock(void*data,FXival count) {
	FXival n;
	do {
		n = io->readBlock(data,count);
		}
	while(n<0 && ((errno==EWOULDBLOCK || errno==EAGAIN) && wait(WaitIO::Readable)));
	return n;
	}


FXlong WaitIO::truncate(FXlong size) {
	return io->truncate(size);
	}

FXbool WaitIO::flush() {
	return io->flush();
	}

FXbool WaitIO::eof() {
	return io->eof();
	}

FXlong WaitIO::size() {
	return io->size();
	}

FXbool WaitIO::close() {
	if (io) {
		io->close();
		delete io;
		io=NULL;
		}
	return true;
	}

FXbool WaitIO::wait(FXuchar mode) {
#ifndef WIN32
	FXint n,nfds=1;
	struct pollfd fds[2];
	fds[0].fd    	= io->handle();
	fds[0].events = (mode==WaitIO::Readable) ? POLLIN : POLLOUT;
	if (watch!=BadHandle) {
		fds[1].fd 	  = watch;
		fds[1].events = POLLIN;
		nfds=2;
		}
	if (timeout) {
		struct timespec ts;
		ts.tv_sec  = (timeout / 1000000000);
		ts.tv_nsec = (timeout % 1000000000);
		do {
			n=ppoll(fds,nfds,&ts,NULL);
			}
		while(n==-1 && (errno==EAGAIN || errno==EINTR));
		}
	else {
		do {
			n=ppoll(fds,nfds,NULL,NULL);
			}
		while(n==-1 && (errno==EAGAIN || errno==EINTR));
		}

	// false if timeout, error or interrupt
	if ((0>=n) || (0<n && watch!=BadHandle && fds[1].revents))
		return false;

	return true;
#endif
	}


}
