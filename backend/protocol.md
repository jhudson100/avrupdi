In the following descriptions, N represents an integer number
given in text (humanly readable) form.
B represents a raw binary byte.

The host sends a command, terminated with a newline character.
The AVR sends its reply as JSON. The JSON response will
have the key "ok" with value true or false depending
on whether the command succeeded. Upon success, other information
may be present as noted below. If an error occurs, the response will
have a "type" key which tells the type of error as well as possible
additional information.

Errors
========

Parity error (type=1)
---------------------

    The parity bit from the target did not match the expected
    value.
    Additional information:
        parity: The received parity value
        stops: The two stop bits received
        value: The data byte received
        msg: Error message

Frame error (type=2)
--------------------
    The start bit or the stop bits from the target
    did not match the expected value.
    Additional information:
        parity: The received parity value
        stops: The two stop bits received
        value: The data byte received
        msg: Error message


Protocol error (type=3)
-----------------------
    A logic error occurred; a command was sent at the wrong
    time or had an invalid parameter.
    Additional information:
        value: An information byte to indicate where
            the problem occurred.
        msg: Error message

Timeout error (type=4)
----------------------
    A response was not received.
    Additional information:
        msg: Error message

Commands
=========



Command
-------

    GET_VERSION

Response
--------

    "version" : N

Explanation
-----------

    Returns the version of the backend.

Command
-------

    CONNECT_UPDI

Response
--------

    No additional information
Explanation
    Connects via UPDI to the target.

Command
-------

    BREAK

Response
--------

    No additional information

Explanation
-----------

    Sends a break signal to the target.

Command
-------

    LDCS N

Response
--------

    "value" : N

Explanation
-----------

    Executes the ldcs command on the target and returns
    the byte response from the target

Command
-----------

    LD N N

Response
----------

    "value" : N

Explanation
-----------

    Executes the ld command on the target and returns the value that
    was read. The first parameter is the size (0=byte, 1=word,
    2=24 bits). The second one is the target: 1 for ptr or 2 for *(ptr++).


Command
-----------

    LDS N

Response
----------

    "value" : N

Explanation
-----------

    Executes the LDS command on the target and returns the value that
    was read. The parameter is the address to read.
    The returned value is always one byte (0...255).


Command
-------

    ST N N N

Response
--------

    No additional information

Explanation
-----------

    Executes the st command on the target. The first argument
    is the pointer type, which may be 1 for *(ptr++)
    or 2 for ptr. The second is the data size, which may
    be 0 for byte, 1 for 16 bits, or 2 for 24 bits.
    The third parameter is the data to store.
    For example, ST 2 1 1000 stores 1000 to the target's
    pointer. A subsequent st 1 0 42 stores the value 42
    to location 1000 in the target.


Command
-------

    STS N N

Response
--------

    No additional information

Explanation
-----------

    Executes the sts command on the target. The first argument
    is the address. The second is the data to store.

Command
-------

    STS_M N N N N N ...

Response
--------

    No additional information

Explanation
-----------

    Executes multiple STS commands on the target.
    The first parameter is the number of address/value pairs
    that follow. After this come the pairs.



Command
-------

    STCS N N

Response
--------

    No additional information


Explanation
-----------

    Executes the stcs command on the target. The first argument
    is the address; the second is the value to store.

Command
-------

    ST_REPEAT N (newline)
    BBB...

Response
--------

    No additional information

Explanation
-----------

    Store (ST) a block of data using the REPEAT prefix. The first
    argument is the number of bytes to send (1-256). After this comes
    the data to send. The host should first send an st command to set
    the target's internal pointer. (For example, "ST 2 1 0" to store 0
    to the pointer.) This command increments the pointer after each
    byte is stored, so once ST has been issued, successive ST_REPEAT
    commands do not require another ST to update the pointer.


Command
-------

    LD_REPEAT N

Response
--------

    "data" : [N,N,N,...]

Explanation
-----------

    Receives a block of data from the target using LD with the REPEAT
    prefix. The argument is the number of bytes to receive (1-256). The
    host should first send an ST command to set the target's internal
    pointer. (For example, "ST 2 1 0" to store 0 to the pointer.) LD_REPEAT
    increments the pointer after each byte is read, so once ST has been
    issued, successive LD_REPEAT commands do not require another ST.

Command
-------

    KEY N N N N N N N N

Response
--------

No additional information

Explanation
-----------

    Send an 8 byte key to the target.

Command
-------

    SIB

Response
--------

    "data" : [ N, N, N, ... ]

Explanation
-----------

    Get the 16 byte SIB from the target.


Command
-------

    TIMEOUT N

Response
--------

    No additional information

Explanation
-----------

    Set the receive timeout from the remote UPDI device in milliseconds.
    The value is a 16 bit unsigned positive integer.


Command
-------

    UPDISPEED N

Response
--------

    No additional information

Explanation
-----------

    Set the bit rate for UPDI communication. Only a preconfigured
    set of bit rates are permitted. See delay.h for the permitted values.



Command
-------

    STS_W N N N N N ...

Response
--------

    No additional information

Explanation
-----------

    Execute multiple STS commands and then wait for several bits of
    a byte in memory to be zero. The first paremeter is the number
    of address/value pairs that follow. After this come the
    address/value pairs. Finally, there is the memory address to poll
    and a mask to be ANDed with the value read.
    For example: Supose the command issued is STS_W 2 100 5 101 6 500 3.
    The 2 indicates there are two address/value pairs. The system
    will store 5 to address 100 and 6  to address 101. Then the system
    will repeatedly poll address 500. Let v be the value read from
    address 500. When v&3 is zero, a response is sent back
    indicating success. If v&3 never becomes zero within the timeout
    interval, a failure message is sent.


Experimental Commands
=======================

These are experimental and are not documented further here due to their
changing nature.

    ST_RV
