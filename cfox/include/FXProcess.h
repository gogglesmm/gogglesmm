/********************************************************************************
*                                                                               *
*                         P r o c e s s   S u p p o r t                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#ifndef FXPROCESS_H
#define FXPROCESS_H

namespace FX {

class FXIODevice;


/// Executable process
class FXAPI FXProcess {
private:
  FXProcessID  pid;     // Handle to process
  FXIODevice  *input;   // Input stream
  FXIODevice  *output;  // Output stream
  FXIODevice  *errors;  // Errors stream
private:
  FXProcess(const FXProcess&);
  FXProcess &operator=(const FXProcess&);
public:

  /// Create and initialize
  FXProcess();

  /// Return handle of this process object
  FXProcessID id() const;

  /// Start subprocess
  FXbool start(const FXchar* exec,const FXchar *const *args,const FXchar *const *env=nullptr);

  /// Change input stream; handle must be Inheritable
  void setInputStream(FXIODevice* is){ input=is; }

  /// Return input stream
  FXIODevice* getInputStream() const { return input; }

  /// Change output stream; handle must be Inheritable
  void setOutputStream(FXIODevice* os){ output=os; }

  /// Return output stream
  FXIODevice* getOutputStream() const { return output; }

  /// Change error stream; handle must be Inheritable
  void setErrorStream(FXIODevice* es){ errors=es; }

  /// Return error stream
  FXIODevice* getErrorStream() const { return errors; }

  /// Get process id of calling process
  static FXint current();

  /// Get current process handle
  static FXInputHandle handle();

  /// Exit calling process
  static void exit(FXint code=0);

  /// Suspend process
  FXbool suspend();

  /// Resume process
  FXbool resume();

  /// Kill process
  FXbool kill();

  /// Wait for child process
  FXbool wait();

  /// Wait for child process, returning exit code
  FXbool wait(FXint& code);

  /// Delete
 ~FXProcess();
  };

}

#endif
