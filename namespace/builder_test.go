package namespace

import (
	"bytes"
	"io"
	"io/ioutil"
	"log"
	"os"
	"path"
	"testing"
	"time"

	"github.com/gojuno/minimock/v3"
	"github.com/stretchr/testify/assert"
)

type args struct {
	ns Namespace
}

func newBUuilder(name string) func(t minimock.Tester) *Builder {
	return func(t minimock.Tester) *Builder {
		wd, err := os.Getwd()
		if err != nil {
			t.Error(err)
			return nil
		}
		f, err := os.Open("testdata/" + name)
		if err != nil {
			t.Error(err)
			return nil
		}
		file, err := Parse(f)
		if err != nil {
			t.Error(err)
			return nil
		}
		return &Builder{
			dir:  wd,
			file: file,
			open: func(path string) (io.Reader, error) { return bytes.NewBuffer(nil), nil },
		}
	}
}
func mockNSBuilder(t minimock.Tester) args { return args{&noopNS{}} }
func TestBuilder_buildNS(t *testing.T) {

	tests := []struct {
		name    string
		init    func(t minimock.Tester) *Builder
		inspect func(r *Builder, t *testing.T) //inspects *Builder after execution of buildNS

		args func(t minimock.Tester) args

		wantErr    bool
		inspectErr func(err error, t *testing.T) //use for more precise error evaluation
	}{}
	files, err := ioutil.ReadDir("testdata")
	if err != nil {
		log.Fatal(err)
	}
	for _, file := range files {
		tests = append(tests, struct {
			name    string
			init    func(t minimock.Tester) *Builder
			inspect func(r *Builder, t *testing.T) //inspects *Builder after execution of buildNS

			args func(t minimock.Tester) args

			wantErr    bool
			inspectErr func(err error, t *testing.T) //use for more precise error evaluation
		}{
			name:    file.Name(),
			init:    newBUuilder(file.Name()),
			args:    mockNSBuilder,
			wantErr: path.Ext(file.Name()) == ".wrong",
		})
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			mc := minimock.NewController(t)
			defer mc.Wait(time.Second)

			tArgs := tt.args(mc)
			receiver := tt.init(mc)

			err := receiver.buildNS(tArgs.ns)

			if tt.inspect != nil {
				tt.inspect(receiver, t)
			}

			if tt.wantErr {
				if assert.Error(t, err) && tt.inspectErr != nil {
					tt.inspectErr(err, t)
				}
			} else {
				assert.NoError(t, err)
			}

		})
	}
}

type noopNS struct{}

func (m *noopNS) Bind(new string, old string, option int) error        { return nil }
func (m *noopNS) Mount(servername, old, spec string, option int) error { return nil }
func (m *noopNS) Unmount(new string, old string) error                 { return nil }
func (m *noopNS) Import(host string, remotepath string, mountpoint string, options int) error {
	return nil
}
func (m *noopNS) Clear() error            { return nil }
func (m *noopNS) Chdir(path string) error { return nil }
