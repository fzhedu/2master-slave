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
  int type = 1;
  string value = "hellow world";
  MasterNode master("127.0.0.1", 8000);
  master.Start();
  //master.Monitor();
  master.SetNotifyHandle(
      type,
      [&]( )->string{ return value;}
  );
  while(true) {
    cin>>value;
    auto ret = master.Notify(type);
    cout <<"notify fail :" << ret.size()<<endl;
  }
}
