export namespace Cpp {
  interface Parameter {
    default_value: string;
    type: string;
    name: string;
  }

  export interface Function {
    name: string;
    return_type: string;
    scope: string[];
    parameters: Parameter[];
  }

  export interface Class {
    bases: Class[];
    constructors: {
      parameters: Parameter[];
    }[];
    methods: Function[];
    scope: string[];
    variables: {
      name: string;
      type: string;
    }[];
  }

  export interface File {
    classes: {[index: string]: Class};
    functions: Function[];
  }
}
