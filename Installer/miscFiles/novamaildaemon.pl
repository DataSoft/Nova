#!/usr/bin/perl

use strict;
use warnings;

use Linux::Inotify2;
use File::ReadBackwards;

$|++;

use sigtrap 'handler' => \&sigIntHandler, 'ABRT', 'QUIT', 'TERM';

$SIG{'INT'} = \&sigIntHandler;

my $file = "/var/log/nova/Nova.log";
my $writefile = "$ENV{'HOME'}/.config/nova/config/attachment.txt";
my $lockfile = "$ENV{'HOME'}/.config/nova/config/maildaemon.lock";
my $notify = new Linux::Inotify2 or die "Unable to create new Inotify2 object: $!";
my $checkline = "";
my $bw = "";
my $add = "";
my $addline = "";
my $templine = "";
my $writelock = 0;

exit 0 if(-e $lockfile);

open my $lockhandle, ">", $lockfile or die "Couldn't create lock file: $!";
close $lockhandle or die "Could not close $lockfile: $!";

$notify->watch($file, IN_MODIFY,
  sub {
    while($writelock == 1){};
    $writelock = 1;
    $bw = File::ReadBackwards->new($file) or die "Cannot read /var/log/nova/Nova.log: $!";
    $addline = $bw->readline;
    $templine = $addline;
    if($checkline eq "")
    {
      $checkline = $addline;
    }
    elsif($checkline ne $addline)
    {
      $add = $bw->readline;
      while(($add ne $checkline) and (not $bw->eof))
      {
        $addline .= $add;
        $add = $bw->readline;
      }
      $checkline = $templine;
    }
    else
    {
      $addline = "";
    }
    # Write $addline to $writefile here
    $bw->close;
    $add = "";
    open my $fileh, ">>", ($writefile) or die "Could not open $writefile: $!";
    print {$fileh} $addline or die "WTF: $!";
    close $fileh or die "Could not close $writefile: $!";
    $addline = "";
    $writelock = 0;
  }
) or die "Watch creation failed: $!";

1 while $notify->poll;

sub sigIntHandler {
  unlink $lockfile;
  exit 0;
}
