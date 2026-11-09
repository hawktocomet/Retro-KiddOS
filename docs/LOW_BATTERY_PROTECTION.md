# Low Battery Protection

KiddOS should monitor battery state while a game is running.

At a configurable low-battery threshold, initially proposed around 15%:

1. Attempt to save an automatic checkpoint.
2. Pause gameplay.
3. Show a child-friendly full-screen charging message.
4. Prevent dismissal through normal Kid Mode input.
5. Wait until external power is detected.
6. Restore the checkpoint and resume where supported.

The child should not have to wait for the battery percentage to rise after
connecting a charger.

A separate critical threshold, initially proposed around 5%, may require
graceful shutdown to protect the device.

Thresholds, battery reporting, charger detection, and safe shutdown behavior
must be validated on actual hardware before release.
