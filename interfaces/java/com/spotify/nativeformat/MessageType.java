package com.spotify.nativeformat;

public enum MessageType {
    None(0),
    Generic(1);

    private int numVal;

    MessageType(int numVal) {
        this.numVal = numVal;
    }

    public int getNumVal() {
        return numVal;
    }
}
