#!/bin/bash
# dumb script to replace multiline "... write to the Free Software Foundation, 
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA." 
# with "see http://www.gnu.org/licenses/>"
# 
# becareful, regex pattern might match too many lines
# 
# written by Andika Triwidada <andika@gmail.com>
# public domain
#
# $Id: fsf-patch.sh,v 1.2 2014/01/08 19:11:38 andika Exp andika $
#
TMPDIR=fsf-tmp
for f in `grep -rH "59 Temple" *|awk -F ':' '{print $1}'|grep -v COPYING|grep -v \.patch$`
do 
    echo $f
    mkdir -p $TMPDIR/`dirname $f`
    sed -n '1h;1!H;${;g;s/write to the.*USA\./see <http:\/\/www.gnu.org\/licenses\/>\./g;p;}' \
	$f > $TMPDIR/$f
done

# done modify files, now relocate them from TMPDIR to proper places
# should be unnecessary if we know how to use sed in-place instead
pushd .
cd $TMPDIR
tar cf - * | (cd ..; tar xf -)
popd

# cleanup
rm -rf $TMPDIR

# post processing:
# check result with 'git diff'
# throw all changes with 'git stash; git stash clear'
