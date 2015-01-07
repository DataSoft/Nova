set knownHostilesFile "hostiles.txt"
set novaProcessName "novad"
set classificationThreshold 0.5

# Make a list of hostile suspects
set knownHostilesTemp [split [read [set fh [open $knownHostilesFile]]] "\n"]; close $fh
set knownHostiles [list]
foreach line $knownHostilesTemp {if {$line != ""} {lappend knownHostiles $line}}


set novaProcess [open "|$novaProcessName |& cat" r+]
fconfigure $novaProcess -blocking 0 -buffering line
fileevent $novaProcess readable [list readNovadOutput $novaProcess]

proc readNovadOutput {chan} {
	if [eof $chan] {
		#puts "Nova process died"
		exit
	}

	gets $chan line
	if {$line != ""} {

		if {[string match "*Done processing PCAP file*" $line]} {
			# wait a bit... it's buggy otherwise
			after 1000
			set suspectList ""
			catch {set suspectList [exec -ignorestderr novacli get all csv]}
			while {$suspectList == ""} {
				puts "ERROR: Novacli failed"
				catch {set suspectList [exec -ignorestderr novacli get all csv]}
			}

			set closeResult [exec -ignorestderr killall -9 novad]



		    # Uncomment for CE threshold testing
			#set ::classificationThreshold 0.0
			#while {$::classificationThreshold < 1} {
				set falsePositives 0
				set truePositives 0

				set suspects [split $suspectList "\n"]
				foreach suspectRow $suspects {
					set suspectData [split $suspectRow ","]

					set suspect [lindex $suspectData 0]
					set classification [lindex $suspectData end]

					if {$classification > $::classificationThreshold} {
						if {[lsearch $::knownHostiles $suspect] == -1} {
							incr falsePositives
						} else {
							incr truePositives
						}
					}
				}

				set truePositiveRate  [expr {1.0*$truePositives/[llength $::knownHostiles]}]
				set falsePositiveRate [expr {1.0*$falsePositives/([llength $suspects] - [llength $::knownHostiles])}]

				#puts "True positives: $truePositives"
				#puts "False positives: $falsePositives"

				#puts "True positive rate: $truePositiveRate"
				#puts "False positive rate: $falsePositiveRate"
				
				puts "$truePositiveRate $falsePositiveRate"

			# Uncomment for CE threshold testing
			#set ::classificationThreshold [expr {$::classificationThreshold + 0.001}]
		    #}


			exit
		}
		puts stderr $line
	}
}

vwait forever
