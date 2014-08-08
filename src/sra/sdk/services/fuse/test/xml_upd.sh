

mkdir mount
rm run.log;

../sra-fuser -F run.log -L debug10 -S 1 ./mount ./fuse-dir.xml
sleep 2

ls -alR ./mount/NotEmptyLink/xsl >ls1 &
touch ./fuse-dir.xml
sleep 3
ls -alR ./mount/NotEmptyLink/xsl >ls2 &
wait

fusermount -u ./mount

grep "SRR027555$" ls1
grep "SRR001334$" ls1
grep "SRR027555$" ls2
grep "SRR001334$" ls2

rm ls1 ls2
rm -rf mount

grep -i updat run.log
