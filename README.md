My attempt at creating a functional programming lanuage for teaching. A mashup of cobol's sentence like structure and basic.

usage (so far)

Variables:

to store data in a variable the keyword is STORE

example

STORE 10 IN x AS NUMBER

this would put the decimal value 10 into a number variable (data types will be covered later)

STORE x in y AS NUMBER

This would store the VALUE of x in y as a number, by default assignments are all by value

STORE x + y IN z AS NUMBER

This would execute x + y and then store the result in z

STORE 10, 20, 30 IN t AS ARRAY OF 3 NUMBERS

Would create a 3 number array and bulk assign the three numbers. Arrays start at 1 in kokoro to make teaching programmign simpler

STORE t's ARRAY ITEM 1 IN b AS NUMBER

Would fetch the value 10 from t's 1st array item and store it in b
