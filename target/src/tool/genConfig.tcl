# genConfig.tcl
#
# modification history
# --------------------
# 01a,16oct01,sn   wrote
# 
# DESCRIPTION
# wtxtcl genConfig.tcl <configlette_name> <obj> ... <obj>
# Output a C file that defines __<configlette_name>Init
# and for each specified file.o references the corresponding
# __file_o marker symbol.

set symbolList {}
set arrayName "__[lindex $argv 0]"
set externSymbolName "__[lindex $argv 0]Init"

foreach arg [lrange $argv 1 [expr [llength $argv] - 1]] {
    if [regexp "(.*)\.o" $arg dummy symbol] {
	regsub -all "\\-" $symbol "_" symbol
	lappend symbolList "__${symbol}_o"
    }
}

foreach symbol $symbolList {
    puts "extern char $symbol;"
}

puts ""
puts "static char * $arrayName \[\] ="
puts "{"
foreach symbol $symbolList {
    puts "    &$symbol,"
}
puts "    0"
puts "};"
puts ""
puts "char ** $externSymbolName ()"
puts "{"
puts "    return $arrayName;"
puts "}"

