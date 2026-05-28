//go:build lib

// Prebuilt mode: link against prebuilt static library.
// Usage: go build -tags lib
// Requires: prebuilt libfinkit_exchange_static.a in build/lib/
package exchange

/*
#cgo CFLAGS: -I${SRCDIR}/include -I${SRCDIR}/modules/platform/include
#cgo CFLAGS: -I${SRCDIR}/include -I${SRCDIR}/modules/ds/include
#cgo CFLAGS: -I${SRCDIR}/include -I${SRCDIR}/modules/sort/include
#cgo CFLAGS: -I${SRCDIR}/include -I${SRCDIR}/modules/stats/include
#cgo linux,amd64   LDFLAGS: -L${SRCDIR}/build/linux_amd64 -lfinkit_exchange_static -lm
#cgo linux,arm64   LDFLAGS: -L${SRCDIR}/build/linux_arm64 -lfinkit_exchange_static -lm
#cgo darwin,amd64  LDFLAGS: -L${SRCDIR}/build/darwin_amd64 -lfinkit_exchange_static -lm
#cgo darwin,arm64  LDFLAGS: -L${SRCDIR}/build/darwin_arm64 -lfinkit_exchange_static -lm
#cgo windows,amd64 LDFLAGS: -L${SRCDIR}/build/windows_amd64 -lfinkit_exchange_static -lm

int fc_init(void);
void fc_cleanup(void);
*/
import "C"

func init() {
	C.fc_init()
}
