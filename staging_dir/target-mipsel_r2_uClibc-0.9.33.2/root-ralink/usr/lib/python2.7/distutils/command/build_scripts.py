"""distutils.command.build_scripts

Implements the Distutils 'build_scripts' command."""

__revision__ = "$Id$"

import os, re
from stat import ST_MODE
from distutils.core import Command
from distutils.dep_util import newer
from distutils.util import convert_path
from distutils import log

# check if Python is called on the first line with this expression
first_line_re = re.compile('^#!.*python[0-9.]*([ \t].*)?$')

class build_scripts (Command):

    description = "\"build\" scripts (copy and fixup #! line)"

    user_options = [
        ('build-dir=', 'd', "directory to \"build\" (copy) to"),
        ('force', 'f', "forcibly build everything (ignore file timestamps"),
        ('executable=', 'e', "specify final destination interpreter path"),
        ]

    boolean_options = ['force']


    def initialize_options (self):
        self.build_dir = None
        self.scripts = None
        self.force = None
        self.executable = None
        self.outfiles = None

    def finalize_options (self):
        self.set_undefined_options('build',
                                   ('build_scripts', 'build_dir'),
                                   ('force', 'force'),
                                   ('executable', 'executable'))
        self.scripts = self.distribution.scripts

    def get_source_files(self):
        return self.scripts

    def run (self):
        if not self.scripts:
            return
        self.copy_scripts()


    def copy_scripts (self):
        """Copy each script listed in 'self.scripts'
        """
        _sysconfig = __import__('sysconfig')
        self.mkpath(self.build_dir)
        outfiles = []
        for script in self.scripts:
            adjust = 0
            script = convert_path(script)
            outfile = os.path.join(self.build_dir, os.path.basename(script))
            outfiles.append(outfile)

            if not self.force and not newer(script, outfile):
                log.debug("not copying %s (up-to-date)", script)
                continue

            # Always open the file, but ignore failures in dry-run mode --
            # that way, we'll get accurate feedback if we can read the
            # script.
            try:
                f = open(script, "r")
            except IOError:
                if not self.dry_run:
                    raise
                f = None
            if f:
                f.close()
            self.copy_file(script, outfile)

        if os.name == 'posix':
            for file in outfiles:
                if self.dry_run:
                    log.info("changing mode of %s", file)
                else:
                    oldmode = os.stat(file)[ST_MODE] & 07777
                    newmode = (oldmode | 0555) & 07777
                    if newmode != oldmode:
                        log.info("changing mode of %s from %o to %o",
                                 file, oldmode, newmode)
                        os.chmod(file, newmode)

    # copy_scripts ()

# class build_scripts
