;/////////////////////////////////////////////////////////////////////////////
;//
;//  CAPSEO - Capseo Video Codec Library
;//  $Id$
;//  (downscales raw BGRA frames)
;//
;//  Authors:
;//      Copyright (c) 2007 by Christian Parpart <trapni@gentoo.org>
;//
;//  This code is based on seom:
;//      (http://neopsis.com/projects/seom/)
;//
;//  This file as well as its whole library is licensed under
;//  the terms of GPL. See the file COPYING.
;//
;/////////////////////////////////////////////////////////////////////////////

BITS 64

SECTION .text

; ---------------------------------------------------------------------------
; INPUT:
;   rdi: frame buffer
;   rsi: current frame width
;   rdx: current frame height
; DESCRIPTION:
;   scales an BGRA frame down
; ---------------------------------------------------------------------------
global scaleBGRA: function
scaleBGRA:
	; some constants (rsi, rcx, mm7)
	imul		rsi, 4			; rsi = width in bytes (line stride)

	imul		rdx, rsi		; rdx = size of buffer in bytes - width * height * 4
	lea		rcx, [rdi + rdx]	; rcx = end of buffer

	pxor		mm7, mm7		; mm7 = 0 (required for punpcklbw)

	; modifiable (r8, rdx, r9)
	mov		r8, rdi			; r8 = 1st (upper) row
	lea		rdx, [r8 + rsi]		; rdx = end of 1st (upper) row

	; process upper+lower row
.processRow:
	lea		r9, [r8 + rsi]		; r9 = 2nd (lower) row (:= upperRow + lineStride)

.processColumn:
	; collect our 2x2 matrix of pixel components (rgba) to average
	;	mm0:mm1  (upper left : upper right)
	;	mm2:mm3  (lower left : lower right)
	movd		mm0, [r8]		; mm0 = current pixel in current row
	punpcklbw	mm0, mm7		; converts each pixel component of byte-size to word-size, in mm0
	movd		mm1, [r8 + 4]		; mm1 = right pixel in current pixel
	punpcklbw	mm1, mm7		; byte-to-word conversion, like done in mm0
	movd		mm2, [r9]		; mm2 = current pixel in lower row
	punpcklbw	mm2, mm7		; byte-to-word conversion, ...
	movd		mm3, [r9 + 4]		; mm3 = right pixel in lower row row

	punpcklbw	mm3, mm7		; convert pixel components from byte to word

	; average the 2x2 pixel matrix
	paddusw		mm0, mm1		; add all 4 pixels from mm0..mm3 into mm0
	paddusw		mm0, mm2		;
	paddusw		mm0, mm3		;
	psrlw		mm0, 2			; /= 4

	packuswb	mm0, mm7		; convert pixel components back from word to byte

	movd		[rdi], mm0		; store averaged pixel in *rdi

	; increment buffer pointers
	add		r8, 8			; increment upper row pointer to next pixel pair
	add		r9, 8			; increment lower row pointer to next pixel pair
	add		rdi, 4			; increment buffer pointer to next pixel-write position
    
	cmp		r8, rdx			; reached end of row?
	jne		.processColumn		; if not, then process next pixel matrix

	cmp		rcx, r9			; reached end of buffer (with lower row)?
	je		.done			; if so, we're done.

	mov		r8, r9			; r8 = 1st row
	lea		rdx, [r8 + rsi]		; rdx = end of 1st row
	jmp		.processRow		; loop back, to process next two rows

.done:
	emms
	ret

SECTION ".note.GNU-stack" noalloc noexec nowrite progbits
; vim:ts=8
