#!/bin/bash

execute()
{
    echo
    echo $1
    eval $1
}

make_random_file()
{
    dd if=/dev/urandom of=$1 bs=1 count=$2
}

make_subdirs()
{
    execute "rm -rf subdir"
    execute "rm -f subdir.kar"
    execute "mkdir subdir"
    execute "mkdir subdir/sub1"
    execute "mkdir subdir/sub2"
    make_random_file "subdir/1.plain" "200000"
    make_random_file "subdir/sub1/2.plain" "200000"
    make_random_file "subdir/sub2/3.plain" "200000"
}

test0()
{
    echo "."
    echo "test 0: no parameters"

    execute "vdb-encrypt"
    execute "vdb-decrypt"
    execute "vdb-validate"
}


test1()
{
    echo "."
    echo "test 1: a missing file"

    execute "vdb-encrypt nonexisting.file"
    if [ "$?" -ne "0" ]; then
      echo "OK: cannot encrypt non-existing file"
    else
      echo "PROBLEM: this should not return OK!"
    fi

    execute "vdb-decrypt nonexisting.file"
    if [ "$?" -ne "0" ]; then
      echo "OK: cannot decrypt non-existing file"
    else
      echo "PROBLEM: this should not return OK!"
    fi

    execute "vdb-validate nonexisting.file"
    if [ "$?" -ne "0" ]; then
      echo "OK: cannot validate non-existing file"
    else
      echo "PROBLEM: this should not return OK!"
    fi
}


test2()
{
    echo "."
    echo "test 2: a missing path"

    execute "vdb-encrypt nonexisting.path/"
    if [ "$?" -ne "0" ]; then
      echo "OK: cannot encrypt non-existing path"
    else
      echo "PROBLEM: this should not return OK!"
    fi

    execute "vdb-decrypt nonexisting.path/"
    if [ "$?" -ne "0" ]; then
      echo "OK: cannot decrypt non-existing path"
    else
      echo "PROBLEM: this should not return OK!"
    fi

    execute "vdb-validate nonexisting.path/"
    if [ "$?" -ne "0" ]; then
      echo "OK: cannot validate non-existing path"
    else
      echo "PROBLEM: this should not return OK!"
    fi
}


test3()
{
    echo "."
    echo "test 3: too many parameters"

    execute "vdb-encrypt param1 param2 param3"
    execute "vdb-decrypt param1 param2 param3"
    execute "vdb-validate param1 param2 param3"
}


test4()
{
    echo "."
    echo "test 4: encrypting a read-only file (should not happed)"
    rm -f random.file
    make_random_file "random.file" "200000"
    chmod -w random.file
    execute "vdb-encrypt random.file"
    execute "vdb-validate random.file"
}


test5()
{
    echo.
    echo "test 5: decrypt a plain-file (nothing should happen)"
    rm -f random.file
    make_random_file "random.file" "200000"
    execute "vdb-decrypt random.file"
    execute "vdb-validate random.file"
}

test6()
{
    execute "vdb-validate /panfs/pan1/sra-test/test/vdb-validate/EMPTY"
    execute "vdb-validate /panfs/pan1/sra-test/test/vdb-validate/EMPTY_SUBS"
    execute "vdb-validate /panfs/pan1/sra-test/test/vdb-validate/SRR393572"
    execute "vdb-validate /panfs/pan1/sra-test/test/vdb-validate/SRR393572.sra"
    execute "vdb-validate /panfs/pan1/sra-test/test/vdb-validate/SRR393572.enc"
    execute "vdb-validate /panfs/pan1/sra-test/test/vdb-validate/NA06986.db"
    execute "vdb-validate /panfs/pan1/sra-test/test/vdb-validate/NA06986.csra"
    execute "vdb-validate /panfs/pan1/sra-test/test/vdb-validate/NA06986.csra.enc"
    execute "vdb-validate /panfs/pan1/sra-test/test/vdb-validate/file.txt"
    execute "vdb-validate /panfs/pan1/sra-test/test/vdb-validate/file.txt.enc"
    execute "vdb-validate /panfs/pan1/sra-test/test/vdb-validate/empty.file"
}


test7()
{
    echo "remove settings-file"
    execute "rm ~/.ncbi/user-settings.mkfg"
    test6

    echo "use settings-file with no password-file"
    echo "#config without password" > ~/.ncbi/user-settings.mkfg
    test6

    echo "use settings-file with not existing password-file"
    echo "#config with not existing password file" > ~/.ncbi/user-settings.mkfg
    echo "/krypto/pwfile = \"\$HOME/.ncbi/none-existing-passwd\"" >> ~/.ncbi/user-settings.mkfg
    test6

    echo "use settings-file with empty password-file"
    echo "#config pointing to empty password file" > ~/.ncbi/user-settings.mkfg
    echo "/krypto/pwfile = \"\$(HOME)/.ncbi/empty-passwd\"" >> ~/.ncbi/user-settings.mkfg
    execute "rm -f ~/.ncbi/empty-passwd"
    execute "touch ~/.ncbi/empty-passwd"
    test6

    echo "use settings-file with wrong password-file"
    echo "#config pointing to password file with wrong password" > ~/.ncbi/user-settings.mkfg
    echo "/krypto/pwfile = \"\$(HOME)/.ncbi/wrong-passwd\"" >> ~/.ncbi/user-settings.mkfg
    echo "wrongpasswd" > ~/.ncbi/wrong-passwd
    test6

    echo "use settings-file with good password-file"
    echo "#config pointing to password file with correct password" > ~/.ncbi/user-settings.mkfg
    echo "/krypto/pwfile = \"\$(HOME)/.ncbi/good-passwd\"" >> ~/.ncbi/user-settings.mkfg
    echo "123456" > ~/.ncbi/good-passwd
    test6

    echo "bring back original settings-file"
    echo "#config pointing to password file with correct password" > ~/.ncbi/user-settings.mkfg
    echo "/krypto/pwfile = \"\$(HOME)/.ncbi/good-passwd\"" >> ~/.ncbi/user-settings.mkfg

    echo "#config pointing to password file" > ~/.ncbi/user-settings.mkfg
    echo "/krypto/pwfile = \"\$(HOME)/.ncbi/vdb-passwd\"" >> ~/.ncbi/user-settings.mkfg

    cd /panfs/pan1/sra-test/test/vdb-validate
    execute "vdb-validate ."
    execute "vdb-validate ./SRR393572"
    cd ..
    execute "vdb-validate vdb-validate/SRR393572"
    execute "rm  ~/.ncbi/empty-passwd  ~/.ncbi/good-passwd  ~/.ncbi/wrong-passwd"
}

test1a()
{
    echo
    echo "test 1a"
    execute "rm -f 1.plain 1.enc 1.dec"
    make_random_file "1.plain" "2000000"
    execute "cp 1.plain 1.enc"
    execute "vdb-encrypt 1.enc"
    execute "vdb-validate 1.enc"
    execute "cp 1.enc 1.dec"
    execute "vdb-decrypt 1.dec"
    execute "diff -qs 1.plain 1.dec"
    execute "filepoke.pl 1.enc 100 1"
    execute "vdb-validate 1.enc"
}


test2a()
{
    echo
    echo "test 2"
    execute "rm -f 2.plain 2.enc 2.dec"
    make_random_file "2.plain" "2000000"
    execute "vdb-encrypt 2.plain 2.enc"
    execute "vdb-validate 2.enc"
    execute "vdb-decrypt 2.enc 2.dec"
    execute "diff -qs 2.plain 2.dec"
}


test3a()
{
    echo
    echo "test 3"
    make_subdirs
    execute "kar -f -d subdir -c subdir.kar"
    execute "vdb-validate subdir.kar"
    execute "vdb-encrypt subdir.kar"
    execute "vdb-validate subdir.kar"
}


test4a()
{
    echo
    echo "test 3"
    make_subdirs
    execute "kar -f -d subdir -c subdir.kar"
    execute "vdb-validate subdir"
    execute "vdb-encrypt subdir"
    execute "vdb-validate subdir"
}

#test0
#test1
#test2
#test3
#test4
#test5

test7
