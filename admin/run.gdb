# SPDX-FileCopyrightText: 2015 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# meant to be run from the build dir
# can be passed a different gemrb.cfg as parameter1
gdb -q -iex "set breakpoint pending on" -iex "b abort" -ex run \
  --args ${0%%/*}/build/gemrb/gemrb -c ${0%%/*}/build/${1:-gemrb.cfg}
