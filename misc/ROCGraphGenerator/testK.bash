for i in {1..17}
do
	novacli writesetting K $i
	printf "%s " $i >> output.txt
	tclsh generate.tcl >> output.txt

	sleep 2
done
