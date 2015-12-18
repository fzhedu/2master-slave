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
 * /2master-slave/gtest_main.cpp
 *
 *  Created on: Dec 17, 2015
 *      Author: imdb
 *		   Email: 
 * 
 * Description:
 *
 */


#include "lib/gtest/gtest.h"
#include <string>
#include "based_node.h"
#include "master_node.h"
#include "slave_node.h"

using std::string;
using std::cin;
using std::cout;
using std::endl;

MasterNode master("127.0.0.1", 8000);
SlaveNode slave1("127.0.0.1", 8001, "127.0.0.1", 8000);
SlaveNode slave2("127.0.0.1", 8002, "127.0.0.1", 8000);
int buffer[100];

/*
 * <127.0.0.1, 7000> is a not exist master
 */
SlaveNode slave3("127.0.0.1", 8002, "127.0.0.1", 7000);

class A{
 public:
 int a;
 int b;
 int c;
 A(){}
 A(int arg1,int arg2,int arg3) {
   a = arg1;
   b = arg2;
   c = arg3;
 }
 private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & a & b & c;
  }
};

class MSTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {

    slave1.SetJobHandle(
        [&](string value) { int x = Derialize<int>(value); buffer[1] = x*x;}
    );
    slave2.SetJobHandle(
        [&](string value) { int x = Derialize<int>(value); buffer[2] = x*x*x;}
    );

    master.SetNotifyHandle(
        1,
        [&]()->string {
          A a(buffer[1],buffer[2],buffer[3]);
          return Serialize<A>(a);
        }
    );

    master.SetNotifyHandle(
        2,
        [&]()->string {
          A a(buffer[4],buffer[5],buffer[6]);
          return Serialize<A>(a);
        }
    );

    slave1.SetUpdateHandle(
        1,
        [&](string value) {
          A a = Derialize<A>(value);
          buffer[1] = a.a;
          buffer[2] = a.b;
          buffer[3] = a.c;
        }
    );

    slave1.SetUpdateHandle(
        2,
        [&](string value) {
          A a = Derialize<A>(value);
          buffer[4] = a.a;
          buffer[5] = a.b;
          buffer[6] = a.c;
        }
    );

    slave2.SetUpdateHandle(
        1,
        [&](string value) {
          A a = Derialize<A>(value);
          buffer[11] = a.a;
          buffer[12] = a.b;
          buffer[13] = a.c;
        }
    );

    slave2.SetUpdateHandle(
        2,
        [&](string value) {
          A a = Derialize<A>(value);
          buffer[14] = a.a;
          buffer[15] = a.b;
          buffer[16] = a.c;
        }
    );
    master.Start();
    usleep(1000);
    slave1.Start();
    slave1.Subscribe(1);
    slave2.Subscribe(2);
    usleep(1000);
    slave2.Start();
    slave2.Subscribe(1);
    slave3.Subscribe(2);
    usleep(1000);
  }
  static void TearDownTestCase() {

  }
};
TEST_F(MSTest, Register) {

  auto ret1 = slave1.Register();
  auto ret2 = slave2.Register();
  //auto ret3 = slave3.Register();
  EXPECT_EQ(0, ret1);
  EXPECT_EQ(0, ret2);
  //EXPECT_EQ(-1,ret3);
}

TEST_F(MSTest, Dispatch) {
  for (int i = 1; i < 5 ; i++) {
    auto ret = master.Dispatch(Addr("127.0.0.1",8001), Serialize<int>(i));
    usleep(1000);
    EXPECT_EQ(i*i,buffer[1]);
  }
}


TEST_F(MSTest, BroadDispatch) {
  for (int i = 1;i < 5;i++) {
    auto slave_list = master.GetLive();
    auto ret = master.BroadDispatch(slave_list, Serialize<int>(i));
    usleep(1000);
    EXPECT_EQ(0, ret.size());
    EXPECT_EQ(i*i, buffer[1]);
    EXPECT_EQ(i*i*i, buffer[2]);
  }
}

TEST_F(MSTest, Subscribe) {
  for (int i = 1;i < 5;i++) {
    buffer[1] = i;
    buffer[2] = i+1;
    buffer[3] = i+2;
    master.Notify(1);
    usleep(1000);
    EXPECT_EQ(buffer[1],buffer[11]);
    EXPECT_EQ(buffer[2],buffer[12]);
    EXPECT_EQ(buffer[3],buffer[13]);

    buffer[4] = i;
    buffer[5] = i-1;
    buffer[6] = i-2;
    master.Notify(2);
    usleep(1000);
    EXPECT_EQ(buffer[4],buffer[14]);
    EXPECT_EQ(buffer[5],buffer[15]);
    EXPECT_EQ(buffer[6],buffer[16]);
  }
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv );
  return RUN_ALL_TESTS();
}



