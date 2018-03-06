;****************************************************************************
;
;  Important! I have used zjm161a module.
;  Changed by Jiao-Xiangfeng jiaoxf@263.net
;
;
;****************************************************************************
		org 0000h
		ljmp main			; main routine entry
		org 0003h
		ljmp ex0srv			; external interrupt 0 entry
		org 000bh
		ljmp tim0srv			; timer 0 overflow entry
		org 0030h
main:		acall LC_INIT			; the LCD initialize
		mov dptr,#msg1
		acall LC_MCT			; display message
		mov dptr,#msg2
		acall LC_MCT			; display message
here:		sjmp here
;
;
;****************************************************************************
;
;  The external interrupt service routine
;
ex0srv:		reti
;		
;****************************************************************************
;
;  The timer0 overflow interrupt service routine
;
tim0srv:	reti
;
;****************************************************************************
;
;  The message to be displayed
;
msg1:		db 'W','E','L','C','O','M','E',03h
msg2:		db 'H','e','l','l','o',03h
;****************************************************************************
;
;  Equates
;
ETX		equ	3			; ASCII ETX Character
E_COLUMNS	equ	12			; Number Of Columns
E_LINES		equ	1			; Number Of Lines
;
LCDEN		bit	p3.0			; LCD Enable
LCDRW		bit	p3.1			; LCD R/W
LCDRS		bit	p3.2			; LCD Register Select
LCDDATA		equ	p1			; LCD Data Register
;
;****************************************************************************
;
;  Data Area
;
B_ONOFF		bit	00h			; OnDisplay /Off Control
B_CURSOR	bit	01h			; Cursor Control
B_BLINK		bit	02h			; Blink Control
D_POS		equ	21h
;
;****************************************************************************
;
;  Description:
;	Initializes Various I/O To A Known State.  
;
;  Entry Requirements:
;	None
;
;  On Exit:
;	None
;
;  Affected:
;	None
;
;  Stack:
;	0 Bytes, Not Including acalled Routines
;
;  Comments:
;	This Should Be acalled Before The Global Interrupts Are Enabled (EA)
;
IO_INIT:	clr	LCDEN			; LCD De-Selected
		setb	LCDRW			; LCD To Read Mode
		setb	LCDRS			; LCD Data Register Selected
		ret				; Return To acaller
;
;****************************************************************************
;
;  Description:
;	Initializes LCD Controller, Clears Display, Homes Cursor, Sets
;	No Blinking, No Display Shift, 5 x 7 Font, 8 Bit Bus.
;
;  Entry Requirements:
;	None
;
;  On Exit:
;	None
;
;  Affected:
;	PSW.CY, PSW.Z, PSW.P, Acc, R2, R3
;
;  Stack:
;	4 Bytes, Not Including acalled Routines
;
;  Comments:
;    Initialized Values:
;       S=0   - No Display Shift
;     I/D=1   - Cursor Moves To Right After Writing A Character
;       D=1   - Display On
;       C=0   - No Cursor Displayed
;       B=0   - No Cursor Character Blink
;      DL=1   - 8 Bit Data Bus
;       N=0   - 1 Display Lines
;       F=0   - 5 x 7 Dots
;
;  The Display Is Cleared, And An Address Is Written Into DD Ram (To
;  Establish That All Subsequent Writes Are Into DD Ram).
;
LC_INIT:	setb	B_ONOFF			; LCD Display On
		clr	B_CURSOR		; Cursor Off
		clr	B_BLINK			; No Blinking
		clr	LCDEN
		acall	l1p1			; Send Init Byte
		acall	l1p1			; Send Init Byte, Again
		acall	l1p1			; Send Init Byte, Again
		acall	l1p1			; Send Init Byte, Again
		acall	LC_DMOD			; Set Display Mode
		acall	LC_CLEAR		; Clear Display
		acall	LC_WAIT			; Wait For Display To Finish
		mov	a,#00000110b   		; Entry Mode, Increase With No Shifting
		acall	LC_IOC			; Strobe Data To Display
		mov	D_POS,#000h		; Set Initial Cursor To 0
		ret				; Return To acaller

l1p1:		mov	a,#00110000b		; Function Set, 8 Bits, 1 Lines, 5 x 7 Dots
		acall	LC_IOC			; Strobe Data To LCD Display
		mov	r3,#0ffh		; R3 = ff
		mov	r2,#0ffh		; R2 = ff
l1p2:		djnz	r3,l1p2			; Loop
		djnz	r2,l1p2			; Waste More Than 4.1 Ms
		ret				; Return To acaller
;
;****************************************************************************
;
;  Description:
;	Displays An ETX Terminated String In Code Space To The LCD.  Does
;	Not Check For A Messages That Are Longer Than The Display.
;
;  Entry Requirements:
;	DPTR Points To ETX Terminated String In Code Space
;
;  On Exit:
;	DPTR Remains Pointing To Start Of String
;
;  Affected:
;	PSW.CY, PSW.Z, PSW.P
;
;  Stack:
;	5 Bytes, Not Including acalled Routines
;
;  Comments:
;	None
;
LC_MCT:		push	acc			; Save Users Acc
		push	dpl			; Save DPL
		push	dph			; Save DPH
;
l2p1:		clr	a			; A = 0, So DPTR + A Gives Correct Data
		movc	a,@a+dptr		; Fetch Character To Output
		inc	dptr			; Increment Data Pointer
	   	cjne	a,#ETX,l2p3		; If ETX Character, Exit
	      	sjmp	l2p2
l2p3:		jz	l2p2			; If A 0x00, Exit
		acall	LC_OUT			; Output Character
		sjmp	l2p1			; Loop Until ETX
;
l2p2:		pop	dph			; Recover DPH
		pop	dpl			; Recover DPL
		pop	acc			; Recover Acc
		ret				; Return If ETX
;
;****************************************************************************
;
;  Description:
;  	Same As LCD_MCT, Only Clears Display First
;
;  Entry Requirements:
;	DPTR Points To ETX Terminated String In Code Space
;
;  On Exit:
;	DPTR Remains Pointing To Start Of String
;
;  Affected:
;	PSW.CY, PSW.Z, PSW.P
;
;  Stack:
;	2 Bytes, Not Including acalled Routines
;
;  Comments:
;	None
;
LC_MCT_CLEAR:	acall	LC_CLEAR		; Clear Display
		acall	LC_MCT			; Display Message
		ret				; Return To acaller
;
;****************************************************************************
;
;  Description:
;	Displays An ETX Terminated String In External Data Space To The LCD.  
;	Does Not Check For A Messages That Are Longer Than The Display.
;
;  Entry Requirements:
;	DPTR Points To ETX Terminated String In External Data Space
;
;  On Exit:
;	DPTR Remains Pointing To Start Of String
;
;  Affected:
;	PSW.CY, PSW.Z, PSW.P
;
;  Stack:
;	5 Bytes, Not Including acalled Routines
;
;  Comments:
;	None
;
LC_MDT:		push	acc			; Save Users Acc
		push	dpl			; Save DPL
		push	dph			; Save DPH
;
l3p1:		movx	a,@dptr			; Fetch Character To Output
		inc	dptr			; Increment Data Pointer
		cjne	a,#ETX,l3p3		; If ETX Character, Exit
	   	sjmp 	l3p2
l3p3:		jz	l3p2			; If A 0x00, Exit
		acall	LC_OUT			; Output Character
		sjmp	l3p1			; Loop Until ETX
;
l3p2:		pop	dph			; Recover DPH
		pop	dpl			; Recover DPL
		pop	acc			; Recover Acc
		ret				; Return If ETX
;
;****************************************************************************
;
;  Description:
;  	Same As LCD_MDT, Only Clears Display First
;
;  Entry Requirements:
;	DPTR Points To ETX Terminated String In External Data Space
;
;  On Exit:
;	DPTR Remains Pointing To Start Of String
;
;  Affected:
;	PSW.CY, PSW.Z, PSW.P
;
;  Stack:
;	2 Bytes, Not Including acalled Routines
;
;  Comments:
;	None
;
LC_MDT_CLEAR:	acall	LC_CLEAR		; Clear Display
		acall	LC_MDT			; Display Message
		ret				; Return To acaller
;
;****************************************************************************
;
;  Description:
; 	Output Character To LCD Display
;
;  Entry Requirements:
;	Acc Has Character To Display 
;
;  On Exit:
;	None
;
;  Affected:
;	PSW.CY
;
;  Stack:
;	2 Bytes, Not Including acalled Routines
;
;  Comments:
;	Increments Cursor Position, But Does Not Check If Running Off Of
;	Display.
;
LC_OUT:		acall	LC_WAIT			; Make Sure Display Not Busy
		acall	LC_IOD			; Strobe Character To LCD Display
		inc	D_POS			; Advance Character By One
		ret				; Return To acaller
;
;****************************************************************************
;
;  Description:
; 	Clear LCD Display To Blanks
;
;  Entry Requirements:
;	None
;
;  On Exit:
;	None
;
;  Affected:
;	PSW.CY
;
;  Stack:
;	3 Bytes, Not Including acalled Routines
;
;  Comments:
;	None
;
LC_CLEAR:	push	acc			; Save Acc
		acall	LC_WAIT			; Make Sure Display Not Busy
		mov	a,#001h			; LCD Clear Display Code
		acall	LC_IOC			; Output Command
		mov	D_POS,#00h		; LCD Cursor To Position 0, 0
		pop	acc			; Recover Acc
		ret				; Return To acaller
;
;****************************************************************************
;
;  Description:
; 	Set LCD Cursor Position
;
;  Entry Requirements:
;	Acc Contains The Character Position.  To Position To Line 2, Acc
;	Would Contain 17.
;
;  On Exit:
;	None
;
;  Affected:
;	PSW.CY
;
;  Stack:
;	3 Bytes, Not Including acalled Routines
;
;  Comments:
;	Tailored To 1x16 Or 2x16 Displays.  Needs Work For Larger Displays.
;
LC_CPOS:	push	acc			; Don't Let User Know We Modified It
		acall	LC_WAIT			; Make Sure LCD Not Busy
		anl	a,#01fh			; Keep Only Low 5 Bits
		jb	acc.4,l4p1		; 0 <= Acc <= F, Fall Through
		mov	D_POS,a			; Store New Cursor Position
		orl	a,#080h			; OR In Set DD Ram Address Function
		acall	LC_IOC			; Strobe Data To Display
		sjmp	l4p2			; Skip To Routine End
;
l4p1:		anl	a,#00fh			; Keep Low Character Position Bits
		orl	a,#040h			; OR In Line 2 Address 
		mov	D_POS,a			; Store New Cursor Position
		orl	a,#080h			; OR In Set DD Ram Address Command
		acall	LC_IOC			; Strobe Data To Display
;
l4p2:		pop	acc			; Recover Acc
		ret				; Return To acaller
;
;****************************************************************************
;
;  Description:
;	Set LCD Display Mode
;
;  Entry Requirements:
;	None
;
;  On Exit:
;	None
;
;  Affected:
;	PSW.CY
;
;  Stack:
;	3 Bytes, Not Including acalled Routines
;
;  Comments:
;	Sets LCD On/Off, Cursor On/Off And Blink/No Blink Mode Based On Bits
;	In Internal Memory.  
;
LC_DMOD:	push	acc			; Save Acc
		acall	LC_WAIT			; Make Sure Display Not Busy
		mov	a,#008h			; Set Mode Control Bit
		mov	c,B_ONOFF		; Get On/Off Control
		mov	acc.2,c			; Set It
		mov	c,B_CURSOR		; Get Cursor Mode
		mov	acc.1,c			; Set It
		mov	c,B_BLINK		; Get Blink Mode
		mov	acc.0,c			; Set It
		acall	LC_IOC			; Strobe Bits To Display
		pop	acc			; Recover Acc
		ret				; Return To acaller
;
;****************************************************************************
;
;  Description:
;	Enable LCD Cursor
;
;  Entry Requirements:
;	None
;
;  On Exit:
;	None
;
;  Affected:
;	PSW.CY
;
;  Stack:
;	2 Bytes, Not Including acalled Routines
;
;  Comments:
;	None
;
LC_CUR_ON:	setb	B_CURSOR		; Enable Cursor
		acall	LC_DMOD			; Do It
		ret				; Return To acaller
;
;****************************************************************************
;
;  Description:
;	Disable LCD Cursor
;
;  Entry Requirements:
;	None
;
;  On Exit:
;	None
;
;  Affected:
;	PSW.CY
;
;  Stack:
;	2 Bytes, Not Including acalled Routines
;
;  Comments:
;	None
;
LC_CUR_OFF:	clr	B_CURSOR		; Disable Cursor
		acall	LC_DMOD			; Do It
		ret				; Return To acaller
;
;****************************************************************************
;
;  Description:
;	Strobe Data In Latch To LCD Command Register
;
;  Entry Requirements:
;	Acc Contains Value To Send To LCD Command Register
;
;  On Exit:
;	None
;
;  Affected:
;	PSW.CY
;
;  Stack:
;	4 Bytes, Not Including acalled Routines
;
;  Comments:
;	None
;
LC_IOC:		push	dpl			; Save DPL
		push	dph			; Save DPH
		clr	LCDRS
		acall	LC_IO			; Load Command Register
		pop	dph			; Recover DPH
		pop	dpl			; Recover DPL
		ret				; Return To acaller
;
;****************************************************************************
;
;  Description:
;	Strobe Data In Latch To LCD Data Register
;
;  Entry Requirements:
;	Acc Contains Value To Send To LCD Data Register
;
;  On Exit:
;	None
;
;  Affected:
;	PSW.CY
;
;  Stack:
;	4 Bytes, Not Including acalled Routines
;
;  Comments:
;	None
;
LC_IOD:		push	dpl			; Save DPL
		push	dph			; Save DPH
		setb	LCDRS
		acall	LC_IO			; Load Data Register
		pop	dph			; Recover DPH
		pop	dpl			; Recover DPL
		ret				; Return To acaller
;
;****************************************************************************
;
;  Description:
;	Strobe Data In Latch To LCD 
;
;  Entry Requirements:
;	Register Select Is Presumed To Be Pointing To Either The Command
;	Or Data Register.  Acc Contains The Value To Write To The Selected
;	Register.
;
;  On Exit:
;	None
;
;  Affected:
;	PSW.CY
;
;  Stack:
;	2 Bytes, Not Including acalled Routines
;
;  Comments:
;	None
;
LC_IO:		mov	LCDDATA,a		; Write To LCD
		clr	LCDRW			; Set Write Mode
		setb	LCDEN			; Set LCD Enable
		clr	LCDEN			; Clear LCD Enable
		ret				; Return To acaller
;
;****************************************************************************
;
;  Description:
;	Delay Until LCD Display Ready
;
;  Entry Requirements:
;	None
;
;  On Exit:
;	None
;
;  Affected:
;	PSW.CY
;
;  Stack:
;	5 Bytes, Not Including acalled Routines
;
;  Comments:
;	Requires That The LCD Be Connected Such That Read Operations Can
;	Be Performed On The Display.
;
LC_WAIT:	push	acc			; Save Acc
		push	dpl			; Save DPL
		push	dph			; Save DPH
		clr	LCDRS			; Set Command Register
		setb	LCDRW			; Set Read Mode
		setb	LCDEN			; Set LCD Enable Line
;
l5p1:		mov	LCDDATA,#0ffh		; Configure As Inputs
		mov	a,LCDDATA		; Read LCD
		jb	acc.7,l5p1		; Loop While Not Ready
;
		clr	LCDEN			; Clear LCD Enable Line
		clr	LCDRW			; Set LCD Back To Write Mode
		pop	dph			; Recover DPH
		pop	dpl			; Recover DPL
		pop	acc			; Recover Acc
		ret				; Return To acaller
;
;****************************************************************************
;
		end
