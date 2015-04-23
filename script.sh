#!/bin/bash   
                                                                  
#simple
#
echo a #
#             

echo hello >a.txt
                         
(cat;(cat ; cat) <b.txt) < a.txt

(echo abd def)

#sequence                                                                       
echo b; echo c

#and                                                                            
(echo b && echo c)

(cat m && echo a)

#or    
(echo b || echo c)

(cat h || echo c)

#indirection                                                                    
echo hello > a

cat a

#exec edge case                                                                 
echo a

 cat h

 #subshell
                                                                       
echo a && (cat h || echo hello) || echo b                                                                

#pipe                                                                           
ls | sort #
#


ls abc | sort

ls | sort abc

