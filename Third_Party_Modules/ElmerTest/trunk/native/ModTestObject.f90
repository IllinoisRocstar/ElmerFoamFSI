MODULE TESTOBJECT

  TYPE t_global
     CHARACTER(80) :: window_name
     CHARACTER(80) :: other_window_name
     CHARACTER(80) :: c_window_name
     INTEGER :: other_window_handle
     INTEGER :: c_window_handle
  END TYPE t_global
  
  CONTAINS
    SUBROUTINE ASSOCIATE_POINTER( attr, ptr)
      TYPE(t_global), POINTER :: attr, ptr
      ptr => attr
    END SUBROUTINE ASSOCIATE_POINTER
    
END MODULE TESTOBJECT
