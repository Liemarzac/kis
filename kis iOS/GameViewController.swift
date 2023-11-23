//
//  GameViewController.swift
//  kis iOS
//
//  Created by Sylvain Gibouret on 21/10/2023.
//

import UIKit
import MetalKit
import KisEngine

// Our iOS specific view controller
class GameViewController: UIViewController {

    var renderer: KisRenderer!
    var mtkView: MTKView!

    override func viewDidLoad() {
        super.viewDidLoad()

        guard let mtkView = self.view as? MTKView else {
            print("View of Gameview controller is not an MTKView")
            return
        }

        // Select the device to render with.  We choose the default device
        guard let defaultDevice = MTLCreateSystemDefaultDevice() else {
            print("Metal is not supported")
            return
        }

        renderer = KisRenderer()

        mtkView.device = defaultDevice
        mtkView.backgroundColor = UIColor.black
        mtkView.delegate = renderer

        let engineParams = UnsafeMutablePointer<kisEngineInitParams>.allocate(capacity: 1)
        engineParams.pointee.m_metalLayer = UnsafeRawPointer(Unmanaged.passUnretained(mtkView.layer).toOpaque())

        var dataPath : String
        if let resourcePath = Bundle.main.resourcePath {
            dataPath = resourcePath
        }
        else {
            dataPath = Bundle.main.bundlePath
        }

        copyStringToCCharArray(swiftString: dataPath, charArray: &engineParams.pointee.m_dataPath)

        kisEngineInit(engineParams)
        engineParams.deallocate()
    }
}
