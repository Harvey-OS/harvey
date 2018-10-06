package main

import (
	"os"
	"fmt"
)

func main() {
	tab := []string{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9" }
	ch := make(chan bool)
	for i := 0; i < 10; i++ {
		go func(i int){
			fmt.Printf("hello %s fmt.printf %d/%d\n", tab[i], i, len(tab))
			ch <- true
		}(i);
	}
	for i := 0; i < 10; i++ {
		<-ch
	}
	os.Stdout.Write([]byte("hello os.write all done\n"))

	if _, err := os.Open("/xyz"); err != nil {
		fmt.Printf("open failed %v\n", err)
	}
}
