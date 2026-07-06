#pragma once

// Full-stop confirmation for Safety mode: makes it unmistakable that the
// transmitter is forcing the ESC to neutral. Static — drawn once on mode entry.
class SafetyScreen {
public:
    void show();
};

extern SafetyScreen safetyScreen;
