/*
Copyright 2018 Harvey OS Team

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package main

import (
	"encoding/binary"
	"fmt"
	"io"
	"os"
)

/*
first word
high 8 bits is ee, which is an invalid address on amd64.
next 8 bits is protocol version
next 16 bits is unused, MBZ. Later, we can make it a packet type.
next 16 bits is core id
next 8 bits is unused
next 8 bits is # words following.

second word is time in ns. (soon to be tsc ticks)

Third and following words are PCs, there must be at least one of them.
*/
type sample struct {
	Wordcount, _    uint8
	Coreid          uint16
	_               uint16
	Version, Marker uint8
	Ns              uint64
}

type backtrace struct {
	Pcs []uint64
}

/* the docs lie. Perl expects Count to be zero. I only wasted a day figuring this out. */
type hdr struct {
	Count, Slots, Format, Period, Padding uint64
}

type record struct {
	Count, Size uint64
	Pcs         []uint64
}

type trailer struct {
	Zero, One, Zeroh uint64
}

func main() {
	var s sample

	r := io.Reader(os.Stdin)
	w := io.Writer(os.Stdout)

	records := make(map[string]uint64, 16384)
	backtraces := make(map[string][]uint64, 1024)

	/* ignore the documentation, it's wrong, first word must be zero.
	 * the perl code that figures word length depends on it.
	 */
	hdr := hdr{0, 3, 0, 10000, 0}
	trailer := trailer{0, 1, 0}
	start := uint64(0)
	end := start
	nsamples := end
	for binary.Read(r, binary.LittleEndian, &s) == nil {
		numpcs := int(s.Wordcount)
		bt := make([]uint64, numpcs)
		binary.Read(r, binary.LittleEndian, &bt)
		//fmt.Printf("%v\n", bt)
		record := ""
		/* Fix the symbols. pprof was unhappy about the 0xfffffff.
		 * N.B. The fact that we have to mess with the bt values
		 * is the reason we did not write a stringer for bt.
		 */
		for i := range bt {
			bt[i] = bt[i] & ((uint64(1) << 32) - 1)
			record = record + fmt.Sprintf("0x%x ", bt[i])
		}
		records[record]++
		backtraces[record] = bt
		//fmt.Printf("%v %d %d %x %v record %v\n", s, s.Wordcount, s.Coreid, s.Ns, bt, record)
		/* how sad, once we go to ticks this gets ugly. */
		if start == 0 {
			start = s.Ns
		}
		end = s.Ns
		nsamples++
	}
	/* we'll need to fix this once we go to ticks. */
	hdr.Period = (end - start) / nsamples
	hdr.Count = uint64(0) // !@$@!#$!@#$len(records))
	//fmt.Printf("start %v end %v nsamples %d period %d\n", start, end, nsamples, hdr.Period)
	binary.Write(w, binary.LittleEndian, &hdr)
	out := make([]uint64, 2)
	/* note that the backtrace length varies. But we're good with that. */
	for key, v := range records {
		bt := backtraces[key]
		out[0] = v
		out[1] = uint64(len(bt))
		dump := append(out, bt...)
		//fmt.Printf("dump %v\n", dump)
		binary.Write(w, binary.LittleEndian, &dump)
	}
	binary.Write(w, binary.LittleEndian, &trailer)

}
