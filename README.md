# CS118_Project2
Daniel Yao
Gloria Chan

This code implements a simple reliable transport on top of UDP
The protocol we're using is Go-Back-N

Go-Back-N basic concept
** Receiver **
1. Doesn't need to keep track of a window
2. Receiver drops all packets not received in sequence
3. ACKS server of the correctly in-order received SEQ #
4. Receiver checks for current SEQ # first before checking last packet flag
5. Packets received by receiver are checked against "packet loss and corruption".
6. If "lost/corrupted" receiver treats as if those packets are never received.

** Sender **
1. Attempts to pre-create all packets needed to transmit requested file.
2. 1 timer for entire CWND
3. Sends all packets in CWND.
4. When an appropriate ACK is received, increment window
5. ACKs/FINs received by sender are checked for "packet loss and corruption".
6. If "lost/corrupted" sender treats as if those packets are never received.
7. Sender goes back to initial state if 5 consecutive time-outs occur. (assumes sender DC-ed)
*/