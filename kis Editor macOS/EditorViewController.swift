//
//  GameViewController.swift
//  kis macOS
//
//  Created by Sylvain Gibouret on 21/10/2023.
//

import Cocoa
import MetalKit
import KisEditor

// Our macOS specific view controller
class EditorViewController: NSViewController {

    var renderer: KisEditorRenderer!

    override func viewDidLoad() {
        super.viewDidLoad()

        guard let mtkView = self.view as? MTKView else {
            print("View attached to GameViewController is not an MTKView")
            return
        }

        // Select the device to render with.  We choose the default device
        guard let defaultDevice = MTLCreateSystemDefaultDevice() else {
            print("Metal is not supported on this device")
            return
        }

        renderer = KisEditorRenderer()

        mtkView.device = defaultDevice
        mtkView.delegate = renderer

        let editorParams = UnsafeMutablePointer<kisEditorInitParams>.allocate(capacity: 1)

        guard let layer = mtkView.layer else {
            print("No metal layer")
            return
        }

        editorParams.pointee.m_metalLayer = UnsafeRawPointer(Unmanaged.passUnretained(layer).toOpaque())

        var dataPath : String
        if let resourcePath = Bundle.main.resourcePath {
            dataPath = resourcePath
        }
        else {
            dataPath = Bundle.main.bundlePath
        }

        copyStringToCCharArray(swiftString: dataPath, charArray: &editorParams.pointee.m_dataPath)

        kisEditorInit(editorParams)
        editorParams.deallocate()
    }
}
