.global _start

_start:
	
	//r0 will have the base address. 
	// Points to leftmost byte
	ldr r0, =input
	
	// Will eventually point to the address of the last char
	ldr r1, =input
	
	//Will hold value of string
	ldrb r2, [r1]
	
	//r4 will be amount of bytes moved/length of string
	mov r4, #0
	
	
	//adresses and/or words
	ldr R7, LED_BASE
	ldr r8, =detected
	ldr r9, =not_detected
	ldr r10, JTAG
	
	
	// if the string is non empty, then we need to iterate
	cmp r2, #0
	bne get_end_of_string
	
	b _exit
	
	

//Here we will get to the end of the string
//need to find end charaters
// Will find end by comparing to the null byte
get_end_of_string:
	add R1, r1, #1
	add r4, r4, #1
	
	ldrb r2, [r1]
	cmp r2, #0
	bne get_end_of_string
	
	// R1 will now hold the address of the null byte
	//must subtract 1 byte to find last character
	sub r1, r1, #1
	
//If the char is a space, we will iterate jump until we find non-space char
//we know that r4 has the length of string. will move until r4 is 1 (string is odd) or 0 (string is even)
check_leftside:
	ldrb r5, [r0]
	sub r4, r4, #1
	add r0, r0, #1
	
	cmp r5, #0x00000020
	beq check_leftside

//Same here, but moving right to left
check_rightside:
	ldrb r6, [r1]
	sub r4, r4, #1
	sub r1, r1, #1
	
	cmp r6, #0x00000020
	beq check_rightside


check_input:
	// If the char is ? then it will have the same value as the char we are comparing it to
	cmp r5, #0x0000003f
	moveq r5, r6
	
	cmp r6, #0x0000003f
	moveq r6, r5
	
	//If the chars are lowercase, which is all chars equal or greater than 97, then we subtract to make lower
	cmp r5, #97
	subge r5, r5, #32
	
	cmp r6, #97
	subge r6, r6, #32
	
	//If the two chars now are NOT equal, the word will never be a palindrome
	cmp r6, r5
	bne not_palindrome
	
	// if the counter is larger than 1 we still need to iterate!
	CMP r4, #1
	bge check_leftside
	b palindrome
	
palindrome:
	ldrb r2, [r8]
	str r2, [r10]
	
	add r8, r8, #1
	
	cmp r2, #0
	bne palindrome
	
	//add newline
	mov r2, #10
	str r2, [r10]
	
	//sets the leds
	add R11, r7, #992
	str r11, [r7]
	
	b _exit

not_palindrome:	
	ldrb r2, [r9]
	str r2, [r10]
	add r9, r9, #1
	
	cmp r2, #0
	bne not_palindrome
	
	mov r2, #10
	str r2, [r10]
	
	
	add R11, r7, #31
	str r11, [r7]
	
	b _exit

JTAG:
	.word 0xFF201000
	
	
//adressen til LEDlys, 
LED_BASE:
	.word 0xFF200000
_exit:
	b .

.data



.align
	input: .asciz "Grav ned den varg"
	
	detected: .asciz "Palindrome detected\n"
	not_detected: .asciz "Not a palindrome\n"
	
	
	
	

.end
