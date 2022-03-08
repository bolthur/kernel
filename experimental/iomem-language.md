# IOMEM Communication Language

Overview of iomem communication language, sort of a more or less simple scripting 
language, for more complex mmio sequences than read, write, wait until 
true / false / not equal / equal. 

This is at the moment just an idea.

## Syntax example

Below is the syntax that is planned to be supported / necessary.

```js
// single line comments
/*
 * multi
 * line
 * comment
 */

// declare some variable
let uint32 a = 5;
let uint32 b;
let uint32 c;

// constant stuff
const uint32 foo = 12345;

// pointer
let ref uint32 p1 = c;

// assign / reassign stuff
a = 10;
b = 2;
p1 = 5;

// calculation with variables
a = a + 1;
a = a - 1;
a = a / 2;
a = a * 2;
// calculation shorthands
a += 1;
a -= 1;
a /= 2;
a *= 2;

// calculation with pointer values
p = p + 1;
p = p - 1;
p = p / 2;
p = p * 2;
// calculation shorthands
p += 1;
p -= 1;
p /= 2;
p *= 2;
// move reference to next / previous position depending on type size
&p += 1;
&p -= 1;

// while loops
while ( a > b ) {
  b += 5;
}

// execute a function from container
let x = mmio_read( 0xA0000000 );

// import an address from container into script bound by name
let ref p2 = import ref some_pointer_name;

// import a constant from container into script bound by name
const bar = import const some_constant_name;
```
