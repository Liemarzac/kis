//
//  ContentView.swift
//  kis Vision
//
//  Created by Sylvain Gibouret on 01/11/2025.
//

import SwiftUI
import RealityKit

struct ContentView: View {

    var body: some View {
        VStack {
            ToggleImmersiveSpaceButton()
        }
    }
}

#Preview(windowStyle: .automatic) {
    ContentView()
        .environment(AppModel())
}
