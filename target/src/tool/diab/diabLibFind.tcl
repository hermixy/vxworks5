# diabLibFind.tcl
#
# modification history
# --------------------
# 01a,24oct01,sn   wrote
# 
# DESCRIPTION
# # dplus -## dummy.o | wtxtcl diabLibFind.tcl <lib>
# Parse the output of dplus -## dummy.o to determine
# the library search path. Then search for library
# <lib> and output found library path to stdout.

set libname [lindex $argv 0]
gets stdin line
regexp {^.* P,([^ ]*) .*} $line dummy link_path
regsub -all ":" $link_path " " link_dirs

set found 0

puts stderr "Searching for $libname in"

foreach dir $link_dirs {
    set lib $dir/$libname
    puts stderr "  $dir ..."
    if [file exists $lib] {
	puts stderr "-> Found $lib"
	puts $lib
	set found 1
	break
    }
}

if { ! $found } {
    puts stderr "Could not find $lib"
    exit 1
}

