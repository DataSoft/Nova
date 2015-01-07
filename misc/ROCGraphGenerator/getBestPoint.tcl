set fh [open [lindex $argv 0]]
set data [split [read $fh] "\n"]
close $fh


set min 9999
foreach line $data {
	if {$line == ""} {continue}

	lassign $line IN TP FP
	set distanceFromBest [expr {sqrt($FP*$FP+((1-$TP)*(1-$TP)))}]

	if {$distanceFromBest < $min} {
		set min $distanceFromBest
		puts $IN
	}
}
