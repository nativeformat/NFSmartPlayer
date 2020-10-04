package com.spotify.nativeformat;

public enum DriverType {
    Soundcard(0),
    File(1);

    private int numVal;

    DriverType(int numVal) {
        this.numVal = numVal;
    }

    public int getNumVal() {
        return numVal;
    }
}
