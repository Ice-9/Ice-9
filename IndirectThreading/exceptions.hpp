
Table 9.2 - THROW code assignments

Code	Reserved for
----	------------
-1	ABORT
-2	ABORT"
-3	stack overflow
-4	stack underflow
-5	return stack overflow
-6	return stack underflow
-7	do-loops nested too deeply during execution
-8	dictionary overflow
-9	invalid memory address
-10	division by zero
-11	result out of range
-12	argument type mismatch
-13	undefined word
-14	interpreting a compile-only word
-15	invalid FORGET
-16	attempt to use zero-length string as a name
-17	pictured numeric output string overflow
-18	parsed string overflow
-19	definition name too long
-20	write to a read-only location
-21	unsupported operation (e.g., AT-XY on a too-dumb terminal)
-22	control structure mismatch
-23	address alignment exception
-24	invalid numeric argument
-25	return stack imbalance
-26	loop parameters unavailable
-27	invalid recursion
-28	user interrupt
-29	compiler nesting
-30	obsolescent feature
-31	>BODY used on non-CREATEd definition
-32	invalid name argument (e.g., TO xxx)
-33	block read exception
-34	block write exception
-35	invalid block number
-36	invalid file position
-37	file I/O exception
-38	non-existent file
-39	unexpected end of file
-40	invalid BASE for floating point conversion
-41	loss of precision
-42	floating-point divide by zero
-43	floating-point result out of range
-44	floating-point stack overflow
-45	floating-point stack underflow
-46	floating-point invalid argument
-47	compilation word list deleted
-48	invalid POSTPONE
-49	search-order overflow
-50	search-order underflow
-51	compilation word list changed
-52	control-flow stack overflow
-53	exception stack overflow
-54	floating-point underflow
-55	floating-point unidentified fault
-56	QUIT
-57	exception in sending or receiving a character
-58	[IF], [ELSE], or [THEN] exception

----------------------------------------------------------------------------
The THROW values {-255...-1} shall be used only as assigned by this Standard.
The values {-4095...-256} shall be used only as assigned by a system.

If the File-Access or Memory-Allocation word sets are implemented, it is
recommended that the non-zero values of ior lie within the range of system
THROW values, as defined above. In an operating-system environment, this can
sometimes be accomplished by biasing the range of operating-system exception-
codes to fall within the THROW range.

Programs shall not define values for use with THROW in the range {-4095...-1}.

