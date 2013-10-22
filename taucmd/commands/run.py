"""
@file
@author John C. Linford (jlinford@paratools.com)
@version 1.0

@brief

This file is part of the TAU Performance System

@section COPYRIGHT

Copyright (c) 2013, ParaTools, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:
 (1) Redistributions of source code must retain the above copyright notice, 
     this list of conditions and the following disclaimer.
 (2) Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation 
     and/or other materials provided with the distribution.
 (3) Neither the name of ParaTools, Inc. nor the names of its contributors may 
     be used to endorse or promote products derived from this software without 
     specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

import sys
import subprocess
import taucmd
from taucmd import util
from taucmd.project import Registry

LOGGER = taucmd.getLogger(__name__)

SHORT_DESCRIPTION = "Gather measurements from an application."

USAGE = """
Usage:
  tau run [<args>...]
"""

HELP = """
'tau run' help page to be written.
"""

def getUsage():
    return USAGE

def getHelp():
    return HELP

def isExecutable(cmd):
    return util.which(cmd) != None

def main(argv):
    """
    Program entry point
    """
    LOGGER.debug('Arguments: %r' % argv)
    cmd = argv[1]
    cmd_args = argv[2:]
    
    registry = Registry()
    if not len(registry.projects):
        print "There are no TAU projects in this directory.  See 'tau project create'."
        return 1

    # Check project compatibility
    proj = registry.getDefaultProject()
    print 'Using TAU project %r' % proj.getName()
    if not proj.supportsExec(cmd):
        print "Warning: %r project may not be compatible with %r." % (proj.getName(), cmd)
        
    # Compile the project if needed
    proj.compile()
    
    # Set the environment
    env = proj.getTauExecEnvironment()
    
    # Get compiler flags
    flags = proj.getTauExecFlags()
    
    # Execute the compiler wrapper script
    cmd = [cmd] + flags + cmd_args

    LOGGER.debug('Creating subprocess: cmd=%r, env=%r' % (cmd, env))
    proc = subprocess.Popen(cmd, env=env, stdout=sys.stdout, stderr=sys.stderr)
    return proc.wait()
    