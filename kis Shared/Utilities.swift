//
//  Utilities.swift
//  kis
//
//  Created by Sylvain Gibouret on 28/10/2023.
//

import Foundation

func copyStringToCCharArray<T>(swiftString : String, charArray : inout T) {
    swiftString.withCString{ (cstr) in
        let srcLen = strlen(cstr)
        let dstSize = MemoryLayout.size(ofValue: charArray)
        let cpySize = min(srcLen, dstSize - 1)
        
        withUnsafeMutablePointer(to: &charArray) { (pCharArray) in
            let rawPointer = UnsafeMutableRawPointer(pCharArray)
            let uint8Pointer = rawPointer.bindMemory(to: UInt8.self, capacity: dstSize)
            uint8Pointer.initialize(to: 0)
            rawPointer.copyMemory(from: cstr, byteCount: cpySize)
        }
    }
}
