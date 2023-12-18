//
//  YNViewController.swift
//  YNKit
//
//  Created by ZHUYN on 2022/1/12.
//

import UIKit

class YNViewController: UIViewController {

    override func viewDidLoad() {
        super.viewDidLoad()

        // Do any additional setup after loading the view.
        self.view.backgroundColor = .gray
        
        let btn = UIButton.init(frame: CGRectMake(100, 200, 100, 50))
        btn.setTitle("NodeList", for: UIControl.State.normal)
        btn.addTarget(self, action: #selector(self.openNodeListVC), for: UIControl.Event.touchUpInside)
        self.view.addSubview(btn)
    }


    @objc func openNodeListVC() {
//        let VC = YNNodeListTestVC.init(nibName: "YNNodeListTestVC", bundle: Bundle.main);
        let VC = YNLinkedListTestVC()
        self.navigationController?.pushViewController(VC, animated: true)
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
