 #!/usr/bin/env bash

# Resets all USB host controllers of the system.
# This is useful in case one stopped working
# due to a faulty device having been connected to it.

base="/sys/bus/pci/drivers"
sleep_secs="0.1"

# This might find a sub-set of these:
# * 'ohci_hcd' - USB 3.0
# * 'ehci-pci' - USB 2.0
# * 'xhci_hcd' - USB 3.0
echo "Looking for USB standards ..."
for usb_std in "$base/"?hci[-_]?c*
do
    echo "* USB standard '$usb_std' ..."
    for dev_path in "$usb_std/"*:*
    do
        dev="$(basename "$dev_path")"
        echo "  - Resetting device '$dev' ..."
        printf '%s' "$dev" | sudo tee "$usb_std/unbind" > /dev/null
        sleep "$sleep_secs"
        printf '%s' "$dev" | sudo tee "$usb_std/bind" > /dev/null
        echo "    done."
    done
    echo "  done."
done
echo "done."