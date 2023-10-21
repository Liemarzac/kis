//
//  GameViewController.swift
//  kis iOS
//
//  Created by Sylvain Gibouret on 21/10/2023.
//

import UIKit
import MetalKit
import engine

// Our iOS specific view controller
class GameViewController: UIViewController {

    var renderer: Renderer!
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

        mtkView.device = defaultDevice
        mtkView.backgroundColor = UIColor.black
//
//        guard let newRenderer = Renderer(metalKitView: mtkView) else {
//            print("Renderer cannot be initialized")
//            return
//        }
//
//        renderer = newRenderer
//
//        renderer.mtkView(mtkView, drawableSizeWillChange: mtkView.drawableSize)

//        mtkView.delegate = renderer

        let p: UnsafeRawPointer = UnsafeRawPointer(Unmanaged.passUnretained(mtkView.layer).toOpaque())
        kisEngine_Init(p)
    }
}
