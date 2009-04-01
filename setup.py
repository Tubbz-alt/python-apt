#! /usr/bin/env python
# $Id: setup.py,v 1.2 2002/01/08 07:13:21 jgg Exp $
import glob
import os
import shutil
import sys

from distutils.core import setup, Extension
from distutils.sysconfig import parse_makefile
from DistUtilsExtra.command import build_extra, build_i18n


class FakeDebianSupportModule(object):
    """Work around the python-apt dependency of debian_support."""

    class Version(object):
        """Empty class."""

sys.modules['debian_bundle.debian_support'] = FakeDebianSupportModule

from debian_bundle.changelog import Changelog



# The apt_pkg module
files = map(lambda source: "python/"+source,
            parse_makefile("python/makefile")["APT_PKG_SRC"].split())
apt_pkg = Extension("apt_pkg", files, libraries=["apt-pkg"])

# The apt_inst module
files = map(lambda source: "python/"+source,
            parse_makefile("python/makefile")["APT_INST_SRC"].split())
apt_inst = Extension("apt_inst", files, libraries=["apt-pkg", "apt-inst"])

# Replace the leading _ that is used in the templates for translation
templates = []

# build doc
if len(sys.argv) > 1 and sys.argv[1] == "build":
    if not os.path.exists("build/data/templates/"):
        os.makedirs("build/data/templates")
    for template in glob.glob('data/templates/*.info.in'):
        source = open(template, "r")
        build = open(os.path.join("build", template[:-3]), "w")
        lines = source.readlines()
        for line in lines:
            build.write(line.lstrip("_"))
        source.close()
        build.close()


if len(sys.argv) > 1 and sys.argv[1] == "clean" and '-a' in sys.argv:
    for dirname in "build/doc", "doc/build", "build/data", "build/mo":
        if os.path.exists(dirname):
            print "Removing", dirname
            shutil.rmtree(dirname)
        else:
            print "Not removing", dirname, "because it does not exist"

setup(name="python-apt",
      version=Changelog(open('debian/changelog')).full_version,
      description="Python bindings for APT",
      author="APT Development Team",
      author_email="deity@lists.debian.org",
      ext_modules=[apt_pkg, apt_inst],
      packages=['apt', 'apt.progress', 'aptsources'],
      data_files = [('share/python-apt/templates',
                    glob.glob('build/data/templates/*.info')),
                    ('share/python-apt/templates',
                    glob.glob('data/templates/*.mirrors'))],
      cmdclass = {"build": build_extra.build_extra,
                  "build_i18n": build_i18n.build_i18n},
      license = 'GNU GPL',
      platforms = 'posix')

if len(sys.argv) > 1 and sys.argv[1] == "build":
    import sphinx
    try:
        import pygtk
    except ImportError:
        print >> sys.stderr, 'E: python-gtk2 is required to build documentation.'
        sys.exit(0)
    sphinx.main(["sphinx", "-b", "html", "-d", "build/doc/doctrees",
                os.path.abspath("doc/source"), "build/doc/html"])
    sphinx.main(["sphinx", "-b", "text", "-d", "build/doc/doctrees",
                os.path.abspath("doc/source"), "build/doc/text"])
