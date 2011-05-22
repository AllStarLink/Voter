/*
* VOTER System Monitor Applet
*
* Copyright (C) 2011
* Jim Dixon, WB6NIL <jim@lambdatel.com>
*
* This file is part of the VOTER System Project
*
*   VOTER System is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 2 of the License, or
*   (at your option) any later version.

*   Voter System is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this project.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * VoterMon.java
 */

package votermon;

import java.awt.Color;
import javax.swing.JLabel;
import javax.swing.JProgressBar;
import javax.swing.JApplet;

/**
 * The applet's main frame.
 */
public class VoterMon extends JApplet {

    private PlayStuff Player = null;
    private JLabel mylabels[];
    private JProgressBar mybars[];
    private String _host = null;
    private String _port = null;

    public VoterMon() {
    }

    // We have to do it this way, because getParameter does not work properly 
    // within a constructor for some reason
    @Override
    public void init () {
        initComponents();
        jButton1.setEnabled(true);
        jButton2.setEnabled(false);
        try {
           _host = getParameter("host");
        }
            catch (Exception e) {
        }
        try {
           _port = getParameter("port");
        }
            catch (Exception e) {
        }
        JLabel tmplabels[] = {jLabel1,jLabel2,jLabel3,jLabel4,jLabel5,
            jLabel6,jLabel7,jLabel8,jLabel9,jLabel10,jLabel11,jLabel12} ;
        mylabels = tmplabels;
        JProgressBar tmpbars[] = {jProgressBar1,jProgressBar2,jProgressBar3,
            jProgressBar4,jProgressBar5,jProgressBar6,jProgressBar7,jProgressBar8,
            jProgressBar9,jProgressBar10,jProgressBar11,jProgressBar12} ;
        mybars = tmpbars;

        for(int i = 0; i < 12; i++) {
            mylabels[i].setText("");
            mylabels[i].setForeground(Color.BLACK);
            mylabels[i].setVisible(false);
            mybars[i].setValue(0);
            mybars[i].setString("0");
            mybars[i].setForeground(Color.BLACK);
            mybars[i].setVisible(false);
         }
    }

    @SuppressWarnings("unchecked")
    private void initComponents() {

        mainPanel = new javax.swing.JPanel();
        jButton1 = new javax.swing.JButton();
        jButton2 = new javax.swing.JButton();
        jPanel1 = new javax.swing.JPanel();
        jLabel1 = new javax.swing.JLabel();
        jLabel2 = new javax.swing.JLabel();
        jLabel3 = new javax.swing.JLabel();
        jLabel4 = new javax.swing.JLabel();
        jLabel5 = new javax.swing.JLabel();
        jLabel6 = new javax.swing.JLabel();
        jProgressBar1 = new javax.swing.JProgressBar();
        jProgressBar2 = new javax.swing.JProgressBar();
        jProgressBar3 = new javax.swing.JProgressBar();
        jProgressBar4 = new javax.swing.JProgressBar();
        jProgressBar5 = new javax.swing.JProgressBar();
        jProgressBar6 = new javax.swing.JProgressBar();
        jLabel7 = new javax.swing.JLabel();
        jLabel8 = new javax.swing.JLabel();
        jLabel9 = new javax.swing.JLabel();
        jLabel10 = new javax.swing.JLabel();
        jLabel11 = new javax.swing.JLabel();
        jLabel12 = new javax.swing.JLabel();
        jProgressBar7 = new javax.swing.JProgressBar();
        jProgressBar8 = new javax.swing.JProgressBar();
        jProgressBar9 = new javax.swing.JProgressBar();
        jProgressBar10 = new javax.swing.JProgressBar();
        jProgressBar11 = new javax.swing.JProgressBar();
        jProgressBar12 = new javax.swing.JProgressBar();

        jButton1.setText("CONNECT"); 
        jButton1.setName("jButton1"); 
        jButton1.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jButton1ActionPerformed(evt);
            }
        });

        jButton2.setText("DISCONNECT"); 
        jButton2.setName("jButton2"); 
        jButton2.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jButton2ActionPerformed(evt);
            }
        });

        jPanel1.setName("jPanel1"); 
        jPanel1.setLayout(new java.awt.GridLayout(4, 6, 10, 10));

        jLabel1.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        jLabel1.setText(""); 
        jLabel1.setName("jLabel1"); 
        jPanel1.add(jLabel1);

        jLabel2.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        jLabel2.setText(""); 
        jLabel2.setName("jLabel2"); 
        jPanel1.add(jLabel2);

        jLabel3.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        jLabel3.setText(""); 
        jLabel3.setName("jLabel3"); 
        jPanel1.add(jLabel3);

        jLabel4.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        jLabel4.setText(""); 
        jLabel4.setName("jLabel4"); 
        jPanel1.add(jLabel4);

        jLabel5.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        jLabel5.setText(""); 
        jLabel5.setName("jLabel5"); 
        jPanel1.add(jLabel5);

        jLabel6.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        jLabel6.setText(""); 
        jLabel6.setName("jLabel6"); 
        jPanel1.add(jLabel6);

        jProgressBar1.setName("jProgressBar1"); 
        jProgressBar1.setString("0"); 
        jProgressBar1.setStringPainted(true);
        jPanel1.add(jProgressBar1);

        jProgressBar2.setName("jProgressBar2"); 
        jProgressBar2.setString("0"); 
        jProgressBar2.setStringPainted(true);
        jPanel1.add(jProgressBar2);

        jProgressBar3.setName("jProgressBar3"); 
        jProgressBar3.setString("0"); 
        jProgressBar3.setStringPainted(true);
        jPanel1.add(jProgressBar3);

        jProgressBar4.setName("jProgressBar4"); 
        jProgressBar4.setString("0"); 
        jProgressBar4.setStringPainted(true);
        jPanel1.add(jProgressBar4);

        jProgressBar5.setName("jProgressBar5"); 
        jProgressBar5.setString("0"); 
        jProgressBar5.setStringPainted(true);
        jPanel1.add(jProgressBar5);

        jProgressBar6.setName("jProgressBar6"); 
        jProgressBar6.setString("0"); 
        jProgressBar6.setStringPainted(true);
        jPanel1.add(jProgressBar6);

        jLabel7.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        jLabel7.setText(""); 
        jLabel7.setName("jLabel7"); 
        jPanel1.add(jLabel7);

        jLabel8.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        jLabel8.setText(""); 
        jLabel8.setName("jLabel8"); 
        jPanel1.add(jLabel8);

        jLabel9.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        jLabel9.setText(""); 
        jLabel9.setName("jLabel9"); 
        jPanel1.add(jLabel9);

        jLabel10.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        jLabel10.setText(""); 
        jLabel10.setName("jLabel10"); 
        jPanel1.add(jLabel10);

        jLabel11.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        jLabel11.setText(""); 
        jLabel11.setName("jLabel11"); 
        jPanel1.add(jLabel11);

        jLabel12.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        jLabel12.setText(""); 
        jLabel12.setName("jLabel12"); 
        jPanel1.add(jLabel12);

        jProgressBar7.setName("jProgressBar7"); 
        jProgressBar7.setString("0"); 
        jProgressBar7.setStringPainted(true);
        jPanel1.add(jProgressBar7);

        jProgressBar8.setName("jProgressBar8"); 
        jProgressBar8.setString("0"); 
        jProgressBar8.setStringPainted(true);
        jPanel1.add(jProgressBar8);

        jProgressBar9.setName("jProgressBar9"); 
        jProgressBar9.setString("0"); 
        jProgressBar9.setStringPainted(true);
        jPanel1.add(jProgressBar9);

        jProgressBar10.setName("jProgressBar10"); 
        jProgressBar10.setString("0"); 
        jProgressBar10.setStringPainted(true);
        jPanel1.add(jProgressBar10);

        jProgressBar11.setName("jProgressBar11"); 
        jProgressBar11.setString("0"); 
        jProgressBar11.setStringPainted(true);
        jPanel1.add(jProgressBar11);

        jProgressBar12.setName("jProgressBar12"); 
        jProgressBar12.setString("0"); 
        jProgressBar12.setStringPainted(true);
        jPanel1.add(jProgressBar12);

        javax.swing.GroupLayout mainPanelLayout = new javax.swing.GroupLayout(mainPanel);
        mainPanel.setLayout(mainPanelLayout);
        mainPanel.setName("mainPanel");

        mainPanelLayout.setHorizontalGroup(
            mainPanelLayout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGroup(mainPanelLayout.createSequentialGroup()
                .addGroup(mainPanelLayout.createParallelGroup(javax.swing.GroupLayout.Alignment.CENTER)
                    .addGroup(mainPanelLayout.createSequentialGroup()
                        .addComponent(jButton1)
                        .addGap(57, 57, 57)
                        .addComponent(jButton2))
                    .addGroup(mainPanelLayout.createSequentialGroup()
                        .addGap(30, 30, 30)
                        .addComponent(jPanel1, javax.swing.GroupLayout.PREFERRED_SIZE, 582, javax.swing.GroupLayout.PREFERRED_SIZE)))
                .addContainerGap(32, Short.MAX_VALUE))
        );
        mainPanelLayout.setVerticalGroup(
            mainPanelLayout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGroup(mainPanelLayout.createSequentialGroup()
                .addContainerGap()
                .addGroup(mainPanelLayout.createParallelGroup(javax.swing.GroupLayout.Alignment.BASELINE)
                    .addComponent(jButton2)
                    .addComponent(jButton1))
                .addGap(18, 18, 18)
                .addComponent(jPanel1, javax.swing.GroupLayout.PREFERRED_SIZE, javax.swing.GroupLayout.DEFAULT_SIZE, javax.swing.GroupLayout.PREFERRED_SIZE)
                .addGap(288, 288, 288))
        );
        this.add(mainPanel);
    }

    private void jButton1ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButton1ActionPerformed
        if (Player == null) {
            Player = new PlayStuff(_host,(short)Integer.parseInt(_port),this);
            Player.start();
            jButton1.setEnabled(false);
            jButton2.setEnabled(true);
        }
    }

    private void jButton2ActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButton2ActionPerformed
            if (Player != null) {
            Player.StopPlay();
            Player = null;
            jButton1.setEnabled(true);
            jButton2.setEnabled(false);
        }
    }

    public void PlayStopped() {
        Player = null;
        jButton1.setEnabled(true);
        jButton2.setEnabled(false);
    }

    public void DoClients(int nclients,String winner,String name[],int rssi[]) {
        int i;

        for(i = 0; i < nclients; i++)
        {
            mylabels[i].setText(name[i]);
            mybars[i].setString(String.valueOf(rssi[i]));
            int pos = ((rssi[i] * 100) + 1) / 256;
            mybars[i].setValue(pos);
            mylabels[i].setVisible(true);
            mybars[i].setVisible(true);
            if (winner.equals(name[i])) {
                mylabels[i].setForeground(Color.RED);
                mybars[i].setForeground(Color.RED);
            } else {
                mylabels[i].setForeground(Color.BLACK);
                mybars[i].setForeground(Color.GRAY);
            }
        }
        for(i = nclients; i < 12; i++) {
            mylabels[i].setText("");
            mylabels[i].setForeground(Color.BLACK);
            mylabels[i].setVisible(false);
            mybars[i].setValue(0);
            mybars[i].setString("0");
            mybars[i].setForeground(Color.BLACK);
            mybars[i].setVisible(false);
        }

    }

    public void DoClients() {
       for(int i = 0; i < 12; i++) {
        mylabels[i].setForeground(Color.BLACK);
        mybars[i].setValue(0);
        mybars[i].setString("0");
        mybars[i].setForeground(Color.BLACK);
       }
     }
 
    private javax.swing.JButton jButton1;
    private javax.swing.JButton jButton2;
    private javax.swing.JLabel jLabel1;
    private javax.swing.JLabel jLabel10;
    private javax.swing.JLabel jLabel11;
    private javax.swing.JLabel jLabel12;
    private javax.swing.JLabel jLabel2;
    private javax.swing.JLabel jLabel3;
    private javax.swing.JLabel jLabel4;
    private javax.swing.JLabel jLabel5;
    private javax.swing.JLabel jLabel6;
    private javax.swing.JLabel jLabel7;
    private javax.swing.JLabel jLabel8;
    private javax.swing.JLabel jLabel9;
    private javax.swing.JPanel jPanel1;
    private javax.swing.JProgressBar jProgressBar1;
    private javax.swing.JProgressBar jProgressBar10;
    private javax.swing.JProgressBar jProgressBar11;
    private javax.swing.JProgressBar jProgressBar12;
    private javax.swing.JProgressBar jProgressBar2;
    private javax.swing.JProgressBar jProgressBar3;
    private javax.swing.JProgressBar jProgressBar4;
    private javax.swing.JProgressBar jProgressBar5;
    private javax.swing.JProgressBar jProgressBar6;
    private javax.swing.JProgressBar jProgressBar7;
    private javax.swing.JProgressBar jProgressBar8;
    private javax.swing.JProgressBar jProgressBar9;
    private javax.swing.JPanel mainPanel;

}
