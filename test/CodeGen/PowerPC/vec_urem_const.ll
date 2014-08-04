; RUN: llc -mcpu=pwr6 -mattr=+altivec < %s

; Common code used to replace the urem by a mulhu, and compilation would
; then crash since mulhu isn't supported on vector types.

define <4 x i32> @test(<4 x i32> %x) {
entry:
  %0 = urem <4 x i32> %x, <i32 2147483647, i32 2147483647, i32 2147483647, i32 2147483647>
  ret <4 x i32> %0
}