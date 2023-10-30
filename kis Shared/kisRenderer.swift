//
//  kisRenderer.swift
//  kis
//
//  Created by Sylvain Gibouret on 29/10/2023.
//
import MetalKit
import KisEngine

class KisRenderer: NSObject, MTKViewDelegate {
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        print("KisRenderer.mtkView\n")
        kisEngineResize(UInt32(size.width), UInt32(size.height))
    }
    
    func draw(in view: MTKView) {
        kisEngineRender()
    }
}
