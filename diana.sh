#!/bin/sh
set -x

input=$0
dianadir="/usr/local"

export LD_LIBRARY_PATH=/metno/local/lib/mesa:$LD_LIBRARY_PATH

export DIANADIR=$dianadir

OPSYS=`uname -s`
debugger="gdb"
SUBNET=`/sbin/ifconfig eth0 | grep "inet addr" | cut -d. -f3`
ulimit -c 100000000000

case $SUBNET in
    20)    region="FOU" ;;
    36|40) region="VV" ;;
    48)    region="VNN" ;;
    16|18|24|26|56|90|104|50) region="VA"  ;;
    *)                         region="VTK" ;;
esac

home=$HOME/.diana
test -d $home || mkdir $home
test -d $home/work  || mkdir $home/work
setup=$dianadir/etc/diana/diana.setup-${region} 
test -e $home/diana.setup  && setup=$home/diana.setup


cd $home

rm core* 1>/dev/null 2>&1
test -s dbxresult  && mv -f dbxresult dbxresult.old
test -s mail.core  && mv -f mail.core mail.core.old

# remove old lpr print files, left after failure...
find . -name "prt_????-??-??_??:??:??.ps" -mtime +1 -exec rm {} \;

tstart=`date`

$dianadir/bin/diana.bin -s $setup -style cleanlooks

tstop=`date`

corefound="no"
for corefile in core*
do
    test -f $corefile || continue
    test -s $corefile || continue
    corefound="yes"
done

if [ $corefound = "yes" ] ; then

echo "USER $USER"     >mail.core
echo "-------------" >>mail.core
uname -a             >>mail.core
echo "-------------" >>mail.core
echo "START $tstart" >>mail.core
echo "STOP  $tstop"  >>mail.core
echo "Diana version (diana.news):" >>mail.core 
cat diana.news       >>mail.core
echo "-------------" >>mail.core
pwd                  >>mail.core
ls -ltr core*        >>mail.core

for corefile in core*
do

test -f $corefile || continue
test -s $corefile || continue

$debugger $dianadir/bin/diana.bin $corefile 1>dbxresult 2>&1 <<ENDDBX
where 
y
y
y
n
q
ENDDBX

echo "===============================================" >>mail.core
cat dbxresult        >>mail.core

rm $corefile

done

echo "===============================================" >>mail.core
hinv                 >>mail.core

adr="audun.christoffersen@met.no helen.korsmo@met.no lisbeth.bergholt@met.no"
Mail -s "DIANA CRASH" $adr < mail.core

fi

exit
