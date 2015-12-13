/*
 * Copyright [2012-2015] DaSE@ECNU
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * /master-slave/main_master.cpp
 *
 *  Created on: Nov 30, 2015
 *      Author: imdb
 *		   Email: 
 * 
 * Description:
 *
 */
#include <iostream>
#include <string>
#include <unistd.h>
#include "master_node.h"
#include "slave_node.h"
#include <semaphore.h>
#include "caf/all.hpp"
#include "caf/io/all.hpp"
using std::string;
using std::cin;
using std::cout;
using std::endl;



int main() {
  int type1 = 1;
  int type2 = 2;
  string x = "hellow world";
  string y = "world hellow";
  MasterNode master("127.0.0.1", 8000);
  master.Start();
  //master.Monitor();
  master.SetNotifyHandle(
      type1,
      [&]()->string { return x;}
  );
  master.SetNotifyHandle(
      type2,
      [&]()->string { return y;}
  );
  int type;
  while(true) {
    cin>>type;
    if (type == type1) {
      cin >> x;
      auto ret = master.Notify(type1);
      cout <<"notify 1 fail :" << ret.size()<<endl;
    }
    else if (type == type2) {
      cin >> y;
      auto ret = master.Notify(type2);
      cout <<"notify 2 fail :" << ret.size()<<endl;
    }
    else if (type == 3) {
       string job;
       cin >> job;
       auto live = master.GetLive();
       cout << "node:"<<live.size() <<endl;
       auto ret = master.BroadDispatch(live, job);
       cout << "fail:"<<ret.size()<<endl;
    }
  }
}
