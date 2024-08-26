.global _start

_start:
	//reset all registers to 0
	MOV R0, #0
	MOV R1, #0
	MOV R2, #0
	MOV r3, #0
	mov r4, #0
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r11, #0
	mov r12, #0
	
	ldr R7, LED_BASE

	// bytes we add/subtract to move up and down address
	mov r3, #1
	
	//r0 will have the base address. DO NOT CHANGE UNTIL ITERATION
	ldr r0, =input
	
	// We will iterate over r1 to find end address.
	ldr r1, =input
	
	// R2 will be used to iterate through
	// the string and check when it ends
	ldrb r2, [r1]
	
	//r4 will be amount of bytes moved/length of string
	mov r4, #0
	
	
	// if the string is non empty, then we need to iterate
	cmp r2, #0
	bne get_end_of_string
	
	b _exit
	
	

//Here we will get to the end of the string
//need to find end charaters
get_end_of_string:
	add R1, r1, r3
	add r4, r4, #1
	
	ldrb r2, [r1]
	cmp r2, #0
	bne get_end_of_string
	
	// R1 will now hold the address of the null byte
	//must subtract 1 byte to find last character

	sub r1, r1, #1
	//sub r4, r4, #1
	// Now we are going to iterate
	// over address and string
	

check_leftside:
	ldrb r5, [r0]
	sub r4, r4, #1
	add r0, r0, #1
	cmp r5, #0x00000020
	beq check_leftside
	
check_rightside:
	ldrb r6, [r1]
	sub r4, r4, #1
	sub r1, r1, #1
	
	cmp r6, #0x00000020
	beq check_rightside
	

	
check_input:
	
	cmp r5, #0x0000003f
	moveq r5, r6
	
	cmp r6, #0x0000003f
	moveq r6, r5
	

	cmp r6, r5
	bne not_palindrome
	
	
	// if the counter is larger than 1 we still need to iterate!
	CMP r4, #1
	bge check_leftside
	
	
	//if r4 is equal, then it is a odd-length string
	//cmp r4, #1
	//bne check_input
	
	b palindrome
	
	
	b _exit
	
palindrome:
	mov r12, #5
	
	add R11, r7, #31
	str r11, [r7]
	
	b _exit

not_palindrome:
	mov r12, #1
	
	add R11, r7, #992
	str r11, [r7]
	
	b _exit
	
	
//adressen til LEDlys, 
LED_BASE:
	.word 0xFF200000
_exit:
	b .

.data



.align
	input: .asciz "ab?a"
	

.end