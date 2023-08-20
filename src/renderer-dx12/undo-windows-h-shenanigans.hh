#pragma once

// After Windows.h has been included we undo some of it's shenanigans here
// Include this file after any files that have Windows.h as a dependency

#undef far
#undef near
#undef pascal