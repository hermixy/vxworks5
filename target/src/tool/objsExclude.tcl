# objsExclude.tcl
#
# modification history
# --------------------
# 01a,24oct01,sn   wrote
# 
# DESCRIPTION
# ar t <lib> | wtxtcl objsExclude.tcl <exclusion-list>
# Output the result of excluding the objects specified
# in <exclusion-list>> from the list of objects contained
# in <lib>.

while {![eof stdin]} {
    gets stdin obj
    set include 1
    foreach exclude_obj $argv {
	if {$obj == $exclude_obj} {
	    set include 0
	    break
	}
    }
    if $include {
	puts $obj
    }
}

