ometa Foo {
	digit = '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9',
	num   = digit+:d -> { type: 'num', val: Number(d.join('')) }
}
