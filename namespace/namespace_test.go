package namespace

import (
	"os"
	"path"
	"testing"
	"time"

	"github.com/gojuno/minimock/v3"
	"github.com/stretchr/testify/assert"
)

type arg struct {
	call syzcall
	flag int
	args []string
}

func checkArgs(t minimock.Tester, args arg, call cmd) error {
	assert.EqualValues(t, args.call, call.syscall, "call number are not equal")
	assert.EqualValues(t, args.args, call.args, "args are not equal")
	assert.Equal(t, args.flag, call.flag, "flags are not equal")
	return nil
}
func TestOPS_NewNS(t *testing.T) {
	type args struct {
		ns Namespace
		c  chan cmd
	}
	tests := []struct {
		name    string
		init    func(t minimock.Tester) File
		inspect func(r File, t *testing.T) //inspects OPS after execution of NewNS

		args func(t minimock.Tester) args

		wantErr    bool
		inspectErr func(err error, t *testing.T) //use for more precise error evaluation
	}{
		{
			name: "simple",
			init: func(t minimock.Tester) File {
				f, _ := os.Open("testdata/namespace.simple")
				ops, _ := Parse(f)
				return ops
			},
		},
		{
			name: "ftp",
			init: func(t minimock.Tester) File {
				f, _ := os.Open("testdata/namespace.ftp")
				ops, _ := Parse(f)
				return ops
			},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			mc := minimock.NewController(t)
			defer mc.Wait(time.Second)
			wd, _ := os.Getwd()
			receiver := tt.init(mc)
			calls := make(chan cmd)

			mock := mockNS{t, calls}
			go func(ops File) {
				for _, call := range ops {
					calls <- call
				}
			}(receiver)
			b := &Builder{
				dir:  path.Join(wd, "testdata"),
				open: open1,
			}
			err := b.buildNS(&mock)

			mc.Finish()
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

type mockNS struct {
	t     *testing.T
	calls chan cmd
}

func (m *mockNS) Bind(new string, old string, option int) error {
	return checkArgs(m.t, arg{
		args: []string{new, old},
		flag: option,
		call: BIND,
	}, <-m.calls)
}

func (m *mockNS) Mount(servername, old, spec string, option int) error {
	return checkArgs(m.t, arg{
		args: []string{servername, old},
		flag: option,
		call: MOUNT,
	}, <-m.calls)
}

func (m *mockNS) Unmount(new string, old string) error {
	return checkArgs(m.t, arg{
		args: []string{new, old},
		flag: 0,
		call: UNMOUNT,
	}, <-m.calls)
}

func (m *mockNS) Import(host string, remotepath string, mountpoint string, options int) error {
	return checkArgs(m.t, arg{
		args: []string{host, remotepath, mountpoint},
		flag: options,
		call: IMPORT,
	}, <-m.calls)
}

func (m *mockNS) Clear() error {
	panic("not implemented") // TODO: Implement
}

func (m *mockNS) Chdir(path string) error {
	return checkArgs(m.t, arg{
		args: []string{path},
		flag: 0,
		call: CHDIR,
	}, <-m.calls)
}
