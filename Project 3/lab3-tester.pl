#! /usr/bin/perl -w

open(FOO, "ospfsmod.c") || die "Did you delete ospfsmod.c?";
$lines = 0;
$lines++ while defined($_ = <FOO>);
close FOO;

@tests = (
    # test reading
    [ 'diff base/hello.txt test/hello.txt >/dev/null 2>&1 && echo $?',
      "0"
    ],
    
    [ 'cmp base/pokercats.gif test/pokercats.gif >/dev/null 2>&1 && echo $?',
      "0"
    ],
        
    [ 'ls -l test/pokercats.gif | awk "{ print \$5 }"',
      "91308"
    ],

    # test writing
    # We use dd to write because it doesn't initially truncate, and it can
    # be told to seek forward to a particular point in the disk.
    [ "echo Bybye | dd bs=1 count=5 of=test/hello.txt conv=notrunc >/dev/null 2>&1 ; cat test/hello.txt",
      "Bybye, world!"
    ],
    
    [ "echo Hello | dd bs=1 count=5 of=test/hello.txt conv=notrunc >/dev/null 2>&1 ; cat test/hello.txt",
      "Hello, world!"
    ],
    
    [ "echo gi | dd bs=1 count=2 seek=7 of=test/hello.txt conv=notrunc >/dev/null 2>&1 ; cat test/hello.txt",
      "Hello, girld!"
    ],
    
    [ "echo worlds galore | dd bs=1 count=13 seek=7 of=test/hello.txt conv=notrunc >/dev/null 2>&1 ; cat test/hello.txt",
      "Hello, worlds galore"
    ],
    
    [ "echo 'Hello, world!' > test/hello.txt ; cat test/hello.txt",
      "Hello, world!"
    ],
    
    # create a file
    [ 'touch test/file1 && echo $?',
      "0"
    ],

    # read directory
    [ 'touch test/dir-contents.txt ; ls test | tee test/dir-contents.txt | grep file1',
      'file1'
    ],

    # write files, remove them, then read dir again
    [ 'ls test | dd bs=1 of=test/dir-contents.txt >/dev/null 2>&1; ' .
      ' touch test/foo test/bar test/baz && '.
      ' rm    test/foo test/bar test/baz && '.
      'diff <( ls test ) test/dir-contents.txt',
      ''
    ],

    # remove the last file
    [ 'rm -f test/dir-contents.txt && ls test | grep dir-contents.txt',
      ''
    ],


    # write to a file
    [ 'echo hello > test/file1 && cat test/file1',
      'hello'
    ],
    
    # append to a file
    [ 'echo hello > test/file1 ; echo goodbye >> test/file1 && cat test/file1',
      'hello goodbye'
    ],

    # delete a file
    [ 'rm -f test/file1 && ls test | grep file1',
      ''
    ],

    # make a larger file for indirect blocks
    [ 'yes | head -n 5632 > test/yes.txt && ls -l test/yes.txt | awk \'{ print $5 }\'',
      '11264'
    ],
   
    # truncate the large file
    [ 'echo truncernated11 > test/yes.txt | ls -l test/yes.txt | awk \'{ print $5 }\' ; rm test/yes.txt',
      '15'
    ],
 
    #NEW TESTS
    #Read first byte of file
    [ 'cat test/hello.txt | dd status=noxfer ibs=1 count=1 2> /dev/null',
      'H'
    ],
    
    #Read first block of file
    [ 'cat test/pokercats.gif | dd status=noxfer ibs=1024 count=1 2>/dev/null > test/cmp;
       cat base/pokercats.gif | dd status=noxfer ibs=1024 count=1 2>/dev/null > base/cmp;
       diff test/cmp base/cmp;
       rm test/cmp base/cmp',
      ''    
    ],

    #Read half of the first block of a file
    [ 'cat test/pokercats.gif | dd status=noxfer ibs=512 count=1 2>/dev/null > test/cmp;
       cat base/pokercats.gif | dd status=noxfer ibs=512 count=1 2>/dev/null > base/cmp;
       diff test/cmp base/cmp;
       rm test/cmp base/cmp',
      ''    
    ],
   
    #Read starting partway through first block, into part of the next
    [ 'cat test/pokercats.gif | dd status=noxfer skip=100 ibs=1024 count=1 2>/dev/null > test/cmp;
       cat base/pokercats.gif | dd status=noxfer skip=100 ibs=1024 count=1 2>/dev/null > base/cmp;
       diff test/cmp base/cmp;
       rm test/cmp base/cmp',
      ''    
    ],

    #Read more than one block
    [ 'cat test/pokercats.gif | dd status=noxfer ibs=1024 count=3 2>/dev/null > test/cmp;
       cat base/pokercats.gif | dd status=noxfer ibs=1024 count=3 2>/dev/null > base/cmp;
       diff test/cmp base/cmp;
       rm test/cmp base/cmp',
      ''    
    ],

    #Read past EOF
    [ 'cat test/hello.txt | dd status=noxfer skip=100 ibs=1024 count=1 2>/dev/null > test/cmp',
       ''    
    ],

    #Test hard links
    [ 'echo foo >> test/foo.txt;
       ln test/foo.txt test/gah.txt;
       cat test/gah.txt',
       'foo'
    ],
    
    [ 'echo blurb > test/gah.txt;
      cat test/foo.txt',
      'blurb'
    ],

    [ 'ln test/foo.txt test/gah2.txt;
      cat test/gah2.txt',
      'blurb'
    ],

    [ 'rm test/foo.txt;
      cat test/gah.txt;
      cat test/gah2.txt;
      rm test/gah.txt test/gah2.txt',
      'blurb blurb'
    ],
  
    #Test Symbolic Links
    [ 'echo foosymbol > test/foo.txt;
      ln -s foo.txt test/gah.txt;
      cat test/gah.txt',
      'foosymbol'
    ],
 
    ['diff test/foo.txt test/gah.txt && echo Same contents',
      'Same contents'
    ],
   
    [ 'echo "World" >> test/foo.txt;
      diff test/foo.txt test/gah.txt && echo Same contents',
      'Same contents'        
    ],

    #Tests for deleting symlinks
    [ 'rm test/gah.txt;
      cat test/foo.txt',
      'foosymbol World'
    ],


   );

my($ntest) = 0;
my(@wanttests);

foreach $i (@ARGV) {
    $wanttests[$i] = 1 if (int($i) == $i && $i > 0 && $i <= @tests);
}

my($sh) = "bash";
my($tempfile) = "lab3test.txt";
my($ntestfailed) = 0;
my($ntestdone) = 0;

foreach $test (@tests) {
    $ntest++;
    next if (@wanttests && !$wanttests[$ntest]);
    $ntestdone++;
    print STDOUT "Running test $ntest\n";
    my($in, $want) = @$test;
    open(F, ">$tempfile") || die;
    print F $in, "\n";
    print STDERR "  ", $in, "\n";
    close(F);
    $result = `$sh < $tempfile 2>&1`;
    $result =~ s|\[\d+\]||g;
    $result =~ s|^\s+||g;
    $result =~ s|\s+| |g;
    $result =~ s|\s+$||;

    next if $result eq $want;
    next if $want eq 'Syntax error [NULL]' && $result eq '[NULL]';
    next if $result eq $want;
    print STDERR "Test $ntest FAILED!\n  input was \"$in\"\n  expected output like \"$want\"\n  got \"$result\"\n";
    $ntestfailed += 1;
}

unlink($tempfile);
my($ntestpassed) = $ntestdone - $ntestfailed;
print "$ntestpassed of $ntestdone tests passed\n";
exit(0);
