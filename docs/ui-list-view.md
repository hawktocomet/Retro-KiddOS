# KiddOS UI ListView

`ListView` is a focusable, vertically navigable list of string entries.
It supports a selected row, configurable row height, automatic scrolling
to keep the selection visible, optional font rendering, and an activation
callback that receives the selected item index.

When focused, controller Up and Down move the selected row; Confirm invokes
the activation callback. Selection stops at the first and last entries.
The list does not own its optional Font; the Font must outlive the widget.
