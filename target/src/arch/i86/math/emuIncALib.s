/* Copyright 1991-2001 Wind River Systems, Inc. */

# modification history
# --------------------
# 01b,07may99,tdl  changed multiline /* */ comment to #
# 01a,24jan95,hdn  original US Software version.

# ***** These are the configuration parameters: *****
#       see below for explanation

EXIRNO	=	16		#interrupt number for exceptions
#EXIRNO	=	2		#interrupt number for exceptions
ENIRS 	=	0x44f6		#enable interrupts
#ENIRS 	=	0x06eb		#skip enabling interrupts
ADDSIZ	=	4		#return address bytes

# interrupt number for exceptions
# This should really always be 16 because that is what the floating-point chip
# uses.  Unfortunately the PC does some funny things to this, both in hardware
# and in software.  It appears that 2 works in some cases.  This is of course
# really NMI.

#        .386P

# offset definitions in the stack frame

s_len	=	72		#frame size 
s_fr	=	s_len+40	#user flag register
s_cs	=	s_len+36	#user instruction pointer
s_ip	=	s_len+32	
s_ax	=	s_len+28	#pushed registers
s_cx	=	s_len+24	
s_dx	=	s_len+20	
s_bx	=	s_len+16	
s_sp	=	s_len+12	
s_bp	=	s_len+8		
s_si	=	s_len+4		
s_di	=	s_len		
s_ds	=	s_len-4		
s_es	=	s_len-8		
s_0	=	0		
iptr	=	4		
memptr	=	8		
index	=	12		
rmcode	=	16		
opcode	=	20		
flag	=	24		
s_A	=	28		
s_B	=	40		
s_X	=	48		

# exponent bias for the three floating-point types

sBIAS	=	0x7f		
lBIAS	=	0x3ff		
tBIAS	=	0x3fff		

# exception definitions

stacku	=	0x41		#stack underflow
stacko	=	0x241		#stack overflow
precis	=	0x20		#precision
underf	=	0x10		#underflow
overf	=	0x08		#overflow
zerod	=	0x04		#zero divide
denorm	=	0x02		#denormal operand
invop	=	0x01		#invalid operation
rup	=	0x200		#rounded up

# macro to load EP number


# macro to store EP number


# macro to save instruction information


# stack pointer macros

				
# macro to handle precision exception


