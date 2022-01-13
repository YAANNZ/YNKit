//
//  YNTabBarController.swift
//  YNKit
//
//  Created by ZHUYN on 2022/1/12.
//

import UIKit

class YNTabBarController: UITabBarController {

    override func viewDidLoad() {
        super.viewDidLoad()

        // Do any additional setup after loading the view.
        self.addChild(UINavigationController.init(rootViewController: YNViewController()))
    }
    

    /*
    // MARK: - Navigation

    // In a storyboard-based application, you will often want to do a little preparation before navigation
    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        // Get the new view controller using segue.destination.
        // Pass the selected object to the new view controller.
    }
    */

}
